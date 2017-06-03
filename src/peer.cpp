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

	bool peer::notify_repliate()
	{
		acl_pthread_mutex_lock(mutex_);
		if (!to_replicate_)
		{
			acl_pthread_cond_signal(cond_);
			to_replicate_ = true;
		}
		acl_pthread_mutex_unlock(mutex_);

	}

	bool peer::notify_vote()
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
		bool to_wait_ = false;
		do
		{
			if (check_do_replicate() || check_heartbeart())
			{
				do_replicate();
			}
			else
			{
				to_wait_ = true;
			}
			if (check_do_vote())
			{
				do_vote();
			}
			else
			{
				to_wait_= true;
			}

			if (to_wait_)
			{
				timespec times;
				times.tv_nsec = 1000*100;//100 milliseconds
				times.tv_sec = 0;
				acl_pthread_cond_timedwait(cond_, mutex_, &times);
			}
		} while (true);
	}

	void peer::do_replicate()
	{
		replicate_log_entries_request req;
		if (node_.build_replicate_log_request(req, next_index_))
		{
			acl::http_rpc_client::get_instance().json_call();
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

		if (heart_inter_ >= diff)
			return true;

		return false;
	}

}