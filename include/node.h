#pragma once
namespace raft
{
	enum result_t
	{
		E_OK,
		E_NO_LEADER,
		E_TIMEOUT,
		E_UNKNOWN,
	};

	struct replicate_waiter_t
	{
		acl_pthread_cond_t *cond_;
		acl_pthread_mutex_t *mutex_;
		result_t result_;
		log_index_t id_;

		replicate_waiter_t()
		{
			acl_pthread_mutexattr_t attr;

			cond_ = acl_thread_cond_create();
			mutex_ = (acl_pthread_mutex_t*)
				acl_mymalloc(sizeof(acl_pthread_mutex_t));
			acl_pthread_mutex_init(mutex_, &attr);

		}
		~replicate_waiter_t()
		{
			acl_pthread_cond_destroy(cond_);
			acl_pthread_mutex_destroy(mutex_);
			acl_myfree(mutex_);
		}
	};

	class node
	{
	public:
		node()
		{

		}
		result_t replicate(const std::string &data, int timeout_millis)
		{
			result_t result;
			log_index_t id = 0;
			int rc = 0;

			replicate_waiter_t *waiter = new replicate_waiter_t;
			
			id = relicate_data(data);;
			add_waiter(id, waiter);

			timespec times;
			times.tv_sec = timeout_millis / 1000;
			times.tv_nsec = 
				(timeout_millis - (long)times.tv_sec*1000) * 1000;

			acl_pthread_mutex_lock(waiter->mutex_);

			if (acl_pthread_cond_timedwait(
				waiter->cond_, 
				waiter->mutex_, 
				&times) == 0)
			{
				result = waiter->result_;
			}
			else if(errno == ETIMEDOUT)
			{
				result = result_t::E_TIMEOUT;
			}
			else
			{
				result = result_t::E_UNKNOWN;
			}
			acl_pthread_mutex_unlock(waiter->mutex_);

			delete waiter;

			return result;
		}
	private:
		friend class peer;


		//for peer
		log_index_t get_last_log_index()
		{
			acl::lock_guard lg(metadata_locker_);
			return last_log_index_;
		}

		term_t get_current_term()
		{

			acl::lock_guard lg(metadata_locker_);
			return current_term_;

		}

		bool build_replicate_log_request(
			replicate_log_entries_request &requst, 
			log_index_t index)
		{

		}

		void replicate_log_callback()
		{

		}

		bool build_vote_request(vote_request &req)
		{

		}

		void vote_response_callback(const vote_response &response)
		{

		}

		void new_term_callback(term_t term)
		{

		}

		bool get_snapshot(std::string &path)
		{

		}
		//


		bool handle_vote_request(const vote_request &req, vote_response &resp)
		{
			return true;
		}

		bool handle_replicate_log_request(
			const replicate_log_entries_request &req, 
			replicate_log_entries_response &resp)
		{

		}

		bool handle_install_snapshot_requst(
			const install_snapshot_request &req, 
			install_snapshot_response &resp)
		{

		}

		void add_waiter(log_index_t id, replicate_waiter_t *waiter)
		{
			replicate_waiters_locker_.lock();
			replicate_waiters_.insert(std::make_pair(id, waiter));
			replicate_waiters_locker_.unlock();
		}
		log_index_t relicate_data(const std::string &data)
		{
			last_log_index_++;
			//...
			return last_log_index_;
		}

		void signal_waiter()
		{
			acl::lock_guard lg(replicate_waiters_locker_);

			std::map<log_index_t, replicate_waiter_t*>::iterator
				it = replicate_waiters_.begin();

			
			for (;it != replicate_waiters_.end();)
			{
				if (it->first > committed_index_)
					break;
				replicate_waiter_t *waiter = it->second;

				acl_pthread_mutex_lock(waiter->mutex_);
				it->second->result_ = result_t::E_OK;
				acl_pthread_cond_signal(waiter->cond_);
				acl_pthread_mutex_unlock(waiter->mutex_);

				it = replicate_waiters_.erase(it);
			}
		}

		void init()
		{

		}
		bool do_commit()
		{

		}


		log_index_t last_log_index_;
		log_index_t committed_index_;
		term_t			  current_term_;
		acl::locker		  metadata_locker_;


		std::map<log_index_t, replicate_waiter_t*>  replicate_waiters_;
		acl::locker replicate_waiters_locker_;

		std::string id_;
	};
}