#include "raft.hpp"
#define  __1MB__      (1024 * 1024)
#define TO_REPLICATE  0x01
#define TO_ELECTION   0x02
#define TO_STOP       0x04

#define SET_TO_REPLICATE(e)   (e |= TO_REPLICATE)
#define SET_TO_ELECTION(e)    (e |= TO_ELECTION)
#define SET_TO_STOP(e)        (e |= TO_STOP)

#define IS_TO_REPLICATE(e)    (e & TO_REPLICATE)
#define IS_TO_STOP(e)         (e & TO_STOP)
#define IS_TO_ELECTION(e)     (e & TO_ELECTION)

#define PEER_SECTION 10


namespace raft
{

	peer::peer(node &_node, const std::string &peer_id)
		:node_(_node),
         peer_id_(peer_id),
         match_index_(0),
         next_index_(0),
         event_(0),
         heart_inter_(3*1000),
         rpc_client_(acl::http_rpc_client::get_instance()),
         rpc_fails_(0),
         req_id_(1)
	{
		//server_id/raft/interface
		replicate_service_path_.format(
			"/memkv%s/raft/replicate_log_req", peer_id_.c_str());

		install_snapshot_service_path_.format(
			"/memkv%s/raft/install_snapshot_req", peer_id_.c_str());

		election_service_path_.format(
			"/memkv%s/raft/vote_req", peer_id_.c_str());

		//send heartbeat to sync log index first
		acl_pthread_mutex_init(&mutex_, NULL);
		acl_pthread_cond_init(&cond_, NULL);

        //init last_replicate_time_
        gettimeofday(&last_replicate_time_, NULL);
    }
	peer::~peer()
	{
		//notify thread to stop
		notify_stop();
		wait();
	}
    void peer::start()
    {
        acl::thread::start();
    }
	void peer::notify_replicate()
	{
		acl_pthread_mutex_lock(&mutex_);
		if (!IS_TO_REPLICATE(event_))
		{
			SET_TO_REPLICATE(event_);
			acl_pthread_cond_signal(&cond_);
		}
		acl_pthread_mutex_unlock(&mutex_);

	}

	void peer::notify_election()
	{
		acl_pthread_mutex_lock(&mutex_);
		if (!IS_TO_ELECTION(event_))
		{
			SET_TO_ELECTION(event_);
			acl_pthread_cond_signal(&cond_);
		}
		acl_pthread_mutex_unlock(&mutex_);
	}
	void peer::set_next_index(log_index_t index)
	{
		acl::lock_guard lg(locker_);
		next_index_ = index;
	}

	void peer::set_match_index(log_index_t index)
	{
		acl::lock_guard lg(locker_);
		match_index_ = index;
	}

	raft::log_index_t peer::match_index()
	{
		acl::lock_guard lg(locker_);
		return match_index_;
	}

	void* peer::run()
	{
		int event = 0;
		while(wait_event(event))
		{
            logger_debug(PEER_SECTION, 10, "event:%x", event);
			if (IS_TO_REPLICATE(event) && node_.is_leader())
			{
				do_replicate();
			}

			if (IS_TO_ELECTION(event))
			{
				do_election();
			}
		} 
		return NULL;
	}

	bool peer::do_install_snapshot()
	{
        logger_debug(PEER_SECTION,10,"trace");

		typedef acl::http_rpc_client::status_t status_t;

		std::string file_path = node_.get_snapshot();
		acl::ifstream file;
		version ver;

		if (file_path.empty())
		{
			logger_error("get snapshot failed");
			return false;
		}
		if (!file.open_read(file_path.c_str()))
		{
			logger_error("open file snapshot failed");
			return false;
		}
		if (!read(file, ver))
		{
			logger_error("snapshot read version failed.");
			return false;
		}
		long long int file_size =  file.fsize();
		long long int offset = 0;

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

			status_t status = rpc_client_.pb_call(
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
				logger("receive new term.%zd",resp.term());
				node_.handle_new_term(resp.term());
				return false;
			}
			offset = resp.bytes_stored();
			//done 
			if (offset == file_size)
			{
				//update next_index
				next_index_ = ver.index_ + 1;
				match_index_ = ver.index_;
				logger("send snapshot done");
				return true;
			}
		}
		return false;
	}
	/*
	 *If last log index  nextIndex for a follower: send 
	 *AppendEntries RPC with log entries starting at nextIndex 
	 *If successful: update nextIndex and matchIndex for follower (��5.3) 
	 *If AppendEntries fails because of log inconsistency: 
	 *decrement nextIndex and retry (��5.3)
	 */
	void peer::do_replicate()
	{
		int entry_size = 1;

		while (node_.is_leader())
		{
			replicate_log_entries_request req;
			replicate_log_entries_response resp;
			acl::http_rpc_client::status_t status;

            logger_debug(PEER_SECTION, 10,
                         "next_index_(%llu)",
                         next_index_);
			if (!node_.build_replicate_log_request(
				req, 
				next_index_, 
				entry_size))
			{
				logger_debug(PEER_SECTION, 10,
                             "build_replicate_log_request "
                             "failed. next_index_:%llu",
                             next_index_);

				if (!do_install_snapshot())
				{
					logger_error("do_install_snapshot error.");
					break;
				}
				continue;
			}
            logger_debug(PEER_SECTION, 2,
                         "term(%lu) "
                         "prev_log_term(%lu) "
                         "prev_log_index(%lu)",
                         req.term(),
                         req.prev_log_term(),
                         req.prev_log_index());

            req.set_req_id(++req_id_);

			//for next heartbeat time;
			gettimeofday(&last_replicate_time_, NULL);

			status = rpc_client_.pb_call(
				replicate_service_path_,
				req,
				resp);

			if (!status)
			{
				logger_error("proto_call error.%s", 
					status.error_str_.c_str());
				rpc_fails_++;
				break;
			}

            logger_debug(PEER_SECTION,10,"replicate done");

			if (!resp.success())
			{
				term_t current_term = node_.current_term();
				if (current_term < resp.term())
				{
                    logger_debug(1, 2,"receive new handle");
					node_.handle_new_term(resp.term());
					break;
				}
				//update next_index.
				next_index_ = resp.last_log_index() + 1;
				entry_size = 1;
				match_index_ = 0;
				continue;
			}
            logger_debug(PEER_SECTION,10,"replicate ok");


			//update peer metadata
			entry_size = 0;
			match_index_ = resp.last_log_index();
			next_index_ = match_index_ + 1;

            //callback to node
            node_.replicate_log_callback();

			//nothings to replicate
			if(next_index_ > node_.last_log_index())
            {
                logger_debug(PEER_SECTION, 10,
                             "next_index_(%llu) "
                                     "last_log_index(%llu).break",
                             next_index_,
                             node_.last_log_index());
                break;
            }

		}		
	}

	void peer::do_election()
	{
		logger("start election");
		if (!node_.is_candidate())
		{
			logger("node is not candidate return");
			return;
		}

		typedef acl::http_rpc_client::status_t status_t;
		vote_request req;
		vote_response resp;
		//async req need req_id_.keep it for the further
		req.set_req_id(++req_id_);
		node_.build_vote_request(req);

        std::string data = req.SerializeAsString();
        acl_assert(data.size());

        logger_debug(PEER_SECTION,10,
                     "req.last_log_index(%lu) "
                     "req.last_log_term(%lu) "
                     "req.term(%lu)",
                     req.last_log_index(),
                     req.last_log_term(),
                     req.term());

		status_t status = rpc_client_.pb_call(
			election_service_path_, 
			req, 
			resp);

		if (!status)
		{
			logger_error("------proto_call error.%s--------",
				status.error_str_.c_str());
			return;
		}

		node_.vote_response_callback(peer_id_, resp);
	}

	bool peer::wait_event(int &event)
	{
        event = 0;

        timespec timeout;

        timeout.tv_sec = last_replicate_time_.tv_sec;
        timeout.tv_nsec = last_replicate_time_.tv_usec * 1000;

        timeout.tv_sec += heart_inter_ / 1000;
        timeout.tv_nsec += heart_inter_ % 1000 * 1000 * 1000;

        acl_pthread_mutex_lock(&mutex_);
        //has event. just do it .don't wait anymore
        if (event_ != 0)
        {
			logger_debug(PEER_SECTION,10,"has event :%d", event);
            event = event_;
            event_ = 0;
            acl_pthread_mutex_unlock(&mutex_);
            return !IS_TO_STOP(event_);
        }

        if(node_.is_leader())
        {
            int status = acl_pthread_cond_timedwait(&cond_,
                                                    &mutex_,
                                                    &timeout);
            if(status == -1)
                logger_error("acl_pthread_cond_timedwait error");
            /*
            * check_heartbeat() for repeat during idle
            * periods to prevent election timeouts (5.2)
            * when cond timeout. it is time to send empty log
            */
            if (status == ACL_ETIMEDOUT)
			{
				logger_debug(PEER_SECTION, 5, "time to send heartbeat msg");
				SET_TO_REPLICATE(event_);
			}
        } else
        {
            //node is not leader.wait without timeout
            acl_pthread_cond_wait(&cond_,&mutex_);
        }
        logger_debug(PEER_SECTION, 10, "event_:%d", event_);
		event = event_;
        event_ = 0;
		acl_pthread_mutex_unlock(&mutex_);

		return !IS_TO_STOP(event_);
	}

	void peer::notify_stop()
	{
		acl_pthread_mutex_lock(&mutex_);
		if (!IS_TO_STOP(event_))
		{
			SET_TO_STOP(event_);
			acl_pthread_cond_signal(&cond_);
			acl_pthread_mutex_unlock(&mutex_);
			return;
		}
		acl_pthread_mutex_unlock(&mutex_);
	}
}
