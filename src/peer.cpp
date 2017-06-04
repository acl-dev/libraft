#include "raft.hpp"
#define  __1MB__ 1024 * 1024
namespace raft
{

	peer::peer(node &_node, const std::string &peer_id)
		:node_(_node),
		match_index_(node_.last_log_index()),
		next_index_(match_index_ + 1),
		peer_id_(peer_id),
		to_replicate_(false),
		to_vote_(false),
		heart_inter_(3*1000),
		rpc_faileds_(0),
		req_id_(1),
		to_stop_(false)
	{
		//server_id/raft/interface
		replicate_service_path_.format(
			"/%s/raft/replicate_log_req", peer_id_.c_str());

		install_snapshot_service_path_.format(
			"/%s/raft/install_snapshot_req", peer_id_.c_str());

		vote_service_path_.format(
			"/%s/raft/vote_req", peer_id_.c_str());


		acl_pthread_mutexattr_t attr;

		//send heartbeat to sync log index first
		cond_ = acl_thread_cond_create();
		mutex_ = (acl_pthread_mutex_t*)
			acl_mymalloc(sizeof(acl_pthread_mutex_t));
		acl_pthread_mutex_init(mutex_, &attr);
		start();
	}

	void peer::notify_repliate()
	{
		acl_pthread_mutex_lock(mutex_);
		if (!to_replicate_)
		{
			acl_pthread_cond_signal(cond_);
			to_replicate_ = true;
		}
		acl_pthread_mutex_unlock(mutex_);

	}

	void peer::notify_vote()
	{
		acl_pthread_mutex_lock(mutex_);
		if (!to_vote_)
		{
			acl_pthread_cond_signal(cond_);
			to_vote_ = true;
		}
		acl_pthread_mutex_unlock(mutex_);
	}

	raft::log_index_t peer::match_index()
	{
		acl::lock_guard lg(locker_);
		return match_index_;
	}

	void* peer::run()
	{
		do
		{
			if ( (check_do_replicate() || check_heartbeart()))
			{
				do_replicate();
			}

			if (check_do_vote())
			{
				do_vote();
			}
			
			to_sleep();

		} while (!check_stop());
	}

	bool peer::do_install_snapshot()
	{
		typedef acl::http_rpc_client::status_t status_t;

		std::string filepath;
		acl::ifstream file;
		version ver;

		if (!node_.get_snapshot(filepath))
		{
			logger_error("get snapshot failed");
			return false;
		}
		if (!file.open_read(filepath.c_str()))
		{
			logger_error("open snapshot failed");
			return false;
		}
		if (!read(file, ver))
		{
			logger_error("snapshot read version failed.");
			return false;
		}
		size_t file_size =  file.fsize();
		size_t offset = 0;

		while (node_.is_leader())
		{
			acl::string buffer(__1MB__);

			if (file.fseek(offset, SEEK_SET) != -1)
			{
				logger_error("file fseek error.%s",acl::last_serror());
				return false;
			}
			file.read(buffer);
			bool done = file_size == (buffer.size() + offset);

			install_snapshot_request req;
			install_snapshot_response resp;

			req.set_data(buffer.c_str(), buffer.size());
			req.set_done(done);
			req.set_offset(offset);
			req.set_leader_id(node_.raft_id());
			req.mutable_snapshot_info()->
				set_last_included_term(ver.term_);
			req.mutable_snapshot_info()->
				set_last_snapshot_index(ver.index_);

			status_t status = rpc_client_->proto_call(
				install_snapshot_service_path_,
				req, 
				resp);
			if (!status)
			{
				logger_error("proto_call error,%s",
					status.error_str_.c_str());
				return false;
			}
			if (node_.current_term() < resp.term())
			{
				logger("recevie new term.%d",resp.term());
				node_.handle_new_term_callback(resp.term());
				return false;
			}
			offset = resp.bytes_stored();
			//done 
			if (offset == file_size)
				return true;
		}
		return false;
	}

	void peer::do_replicate()
	{
		int entry_size = 1;

		while (node_.is_leader())
		{
			replicate_log_entries_request req;
			replicate_log_entries_response resp;
			acl::http_rpc_client::status_t status;

			if (!node_.build_replicate_log_request(
				req, 
				next_index_, 
				entry_size))
			{
				logger("build_replicate_log_request "
					"failed. next_index_:%d",
					next_index_);
				if (!do_install_snapshot())
				{
					logger_error("do_install_snapshot error.");
					break;
				}
				continue;
					
			}
			status = rpc_client_->proto_call(
				replicate_service_path_,
				req,
				resp);
			if (!status)
			{
				logger_error("proto_call error.%s", 
					status.error_str_.c_str());
				rpc_faileds_++;
				break;
			}

			if (!resp.success())
			{
				term_t current_term = node_.current_term();
				if (current_term < resp.term())
				{
					node_.handle_new_term_callback(resp.term());
					break;
				}
				//update next_index.
				next_index_ = resp.last_log_index() + 1;
				entry_size = 1;
				match_index_ = 0;
				continue;
			}
			//for next heartbeat time;
			gettimeofday(&last_replicate_time_, NULL);

			node_.replicate_log_callback();

			//
			entry_size = 0;
			match_index_ = resp.last_log_index();
			next_index_ = match_index_ + 1;
			
			//nothings to replicate
			if(next_index_ > node_.last_log_index())
				break;

		}
		
	}

	void peer::do_vote()
	{
		if (!node_.is_candicate())
			return;
		typedef acl::http_rpc_client::status_t status_t;
		vote_request req;
		vote_response resp;
		//async req need req_id_.keep it for the further
		req.set_req_id(req_id_);
		node_.build_vote_request(req);

		status_t status = rpc_client_->proto_call(
			vote_service_path_, 
			req, 
			resp);

		if (!status)
		{
			logger_error("proto_call error.%s",
				status.error_str_.c_str());
			return;
		}

		node_.vote_response_callback(peer_id_, resp);
	}

	bool peer::check_do_replicate()
	{
		acl_pthread_mutex_lock(mutex_);
		if (to_replicate_)
		{
			to_replicate_ = false;
			acl_pthread_mutex_unlock(mutex_);
			return true;
		}
		acl_pthread_mutex_unlock(mutex_);
		return false;
	}

	bool peer::check_do_vote()
	{
		acl_pthread_mutex_lock(mutex_);		
		if (to_vote_)
		{
			to_vote_ = false;
			acl_pthread_mutex_unlock(mutex_);
			return true;
		}
		acl_pthread_mutex_unlock(mutex_);
		return false;
	}

	bool peer::check_heartbeart()
	{
		timeval now;

		gettimeofday(&now, NULL);
		long long diff = now.tv_sec - last_replicate_time_.tv_sec;
		diff *= 1000;
		diff += (now.tv_usec - now.tv_usec) / 1000;

		if (diff <= heart_inter_)
			return false;
		return true;
	}

	void peer::to_sleep()
	{
		timespec times;

		times.tv_nsec = 1000 * 100;//100 milliseconds
		times.tv_sec = 0;
		acl_pthread_mutex_lock(mutex_);
		acl_pthread_cond_timedwait(cond_, mutex_, &times);
		acl_pthread_mutex_unlock(mutex_);
	}

	void peer::notify_stop()
	{
		acl_pthread_mutex_lock(mutex_);
		if (!to_stop_)
		{
			to_stop_ = true;
			acl_pthread_cond_signal(cond_);
			acl_pthread_mutex_unlock(mutex_);
			//wait for thread to eixt;
			wait();
			return;
		}
		acl_pthread_mutex_unlock(mutex_);
	}

	bool peer::check_stop()
	{
		acl_pthread_mutex_lock(mutex_);
		if (to_stop_)
		{
			acl_pthread_mutex_unlock(mutex_);
			return true;
		}
		acl_pthread_mutex_unlock(mutex_);
		return false;
	}

}