#include "raft.hpp"
namespace raft
{

	peer::peer(node &_node)
		:node_(_node)
	{
		acl_pthread_mutexattr_t attr;

		match_index_ = 0;
		next_index_ = 0;

		cond_ = acl_thread_cond_create();
		mutex_ = (acl_pthread_mutex_t*)
			acl_mymalloc(sizeof(acl_pthread_mutex_t));
		acl_pthread_mutex_init(mutex_, &attr);

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

	void* peer::run()
	{
		do
		{
			if ( (check_do_replicate() ||check_heartbeart()) && 
				node_.is_leader() )
			{
				do_replicate();
			}

			if (check_do_vote() && node_.is_candicate())
			{
				do_vote();
			}
		} while (true);
	}

	bool peer::do_install_snapshot()
	{
		std::string filepath;
		
		if (!node_.get_snapshot(filepath))
		{
			logger_error("get_snapshot failed");
			return false;
		}

		while (node_.is_leader())
		{

		}
		return true;
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
					"failed.next_index_:%d",
					next_index_);
				do_install_snapshot();
				continue;
			}
			status = rpc_client_->proto_call(
				replicate_service_path_.c_str(),
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
				term_t current_term = node_.get_current_term();
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

			//
			entry_size = 0;
			match_index_ = resp.last_log_index();
			next_index_ = match_index_ + 1;
			
			if(next_index_ >= node_.last_log_index())
				break;

		}
		
	}

	void peer::do_vote()
	{

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

}