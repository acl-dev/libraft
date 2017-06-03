#include "raft.hpp"
namespace raft
{
	replicate_waiter_t::replicate_waiter_t()
	{
		acl_pthread_mutexattr_t attr;

		cond_ = acl_thread_cond_create();
		mutex_ = (acl_pthread_mutex_t*)
			acl_mymalloc(sizeof(acl_pthread_mutex_t));
		acl_pthread_mutex_init(mutex_, &attr);
	}

	replicate_waiter_t::~replicate_waiter_t()
	{
		acl_pthread_cond_destroy(cond_);
		acl_pthread_mutex_destroy(mutex_);
		acl_myfree(mutex_);
	}

	node::node()
	{

	}

	raft::result_t node::
		replicate(const std::string &data, int timeout_millis)
	{
		result_t result;
		log_index_t index = 0;
		int rc = 0;

		if (!write_log(data, index))
		{
			logger_fatal("write_log error");
			return E_UNKNOWN;
		}

		replicate_waiter_t *waiter = new replicate_waiter_t;
		add_waiter(index, waiter);

		notify_peers_replicate_log();

		timespec times;
		times.tv_sec = timeout_millis / 1000;
		times.tv_nsec =
			(timeout_millis - (long)times.tv_sec * 1000) * 1000;

		acl_pthread_mutex_lock(waiter->mutex_);

		if (acl_pthread_cond_timedwait(
			waiter->cond_,
			waiter->mutex_,
			&times) == 0)
		{
			result = waiter->result_;
		}
		else if (errno == ETIMEDOUT)
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

	raft::log_index_t node::get_last_log_index()
	{
		acl::lock_guard lg(metadata_locker_);
		return last_log_index_;
	}

	raft::term_t node::get_current_term()
	{
		acl::lock_guard lg(metadata_locker_);
		return current_term_;
	}

	raft::log_index_t node::last_log_index()
	{
		acl::lock_guard lg(metadata_locker_);
		return last_log_index_;
	}

	bool node::build_replicate_log_request(
		replicate_log_entries_request &requst, 
		log_index_t index)
	{
		requst.set_leader_id(raft_id_);
		requst.set_leader_commit(committed_index_);

		if (index <= last_log_index())
		{
			std::vector<log_entry> entries;
			//index -1 for prev_log_term, set_prev_log_index
			if (log_.read(index - 1, __10M__, __100__, entries))
			{
				requst.set_prev_log_index(entries[0].index());
				requst.set_prev_log_term(entries[0].index());
				//first one is prev log
				for (size_t i = 1; i < entries.size(); i++)
				{
					//copy
					*requst.add_entries() = entries[i];
				}
				// read log ok
				return true;
			}
		}
		else
		{
			log_entry entry;
			/*index -1 for prev_log_term, prev_log_index */
			if (log_.read(index - 1, entry))
			{
				requst.set_prev_log_index(entry.index());
				requst.set_prev_log_term(entry.index());

				// read log ok
				return true;
			}
		}
		//read log failed
		return false;
	}

	void node::replicate_log_callback()
	{

	}

	bool node::build_vote_request(vote_request &req)
	{

	}

	void node::vote_response_callback(const vote_response &response)
	{

	}

	void node::new_term_callback(term_t term)
	{

	}

	bool node::get_snapshot(std::string &path)
	{

	}

	void node::notify_peers_replicate_log()
	{
		acl::lock_guard lg(peers_locker_);
		std::map<std::string, peer *>::iterator it =
			peers_.begin();
		for (; it != peers_.end(); ++it)
		{
			it->second->notify_repliate();
		}
	}

	bool node::handle_vote_request(const vote_request &req, 
		vote_response &resp)
	{
		return true;
	}

	bool node::handle_replicate_log_request(
		const replicate_log_entries_request &req, 
		replicate_log_entries_response &resp)
	{

	}

	bool node::handle_install_snapshot_requst(
		const install_snapshot_request &req, 
		install_snapshot_response &resp)
	{

	}

	void node::add_waiter(log_index_t id, replicate_waiter_t *waiter)
	{
		replicate_waiters_locker_.lock();
		replicate_waiters_.insert(std::make_pair(id, waiter));
		replicate_waiters_locker_.unlock();
	}

	void node::make_log_entry(const std::string &data, log_entry &entry)
	{
		metadata_locker_.lock();
		++last_log_index_;
		metadata_locker_.lock();

		entry.set_index(last_log_index_);
		entry.set_term(current_term_);
		entry.set_log_data(data);
		entry.set_type(log_entry_type::e_raft_log);
	}

	bool node::write_log(const std::string &data, log_index_t &index)
	{

		log_entry entry;

		make_log_entry(data, entry);

		if (!log_.write(entry))
		{
			logger_fatal("log write error");
			return false;
		}
		index = entry.index();
		return true;
	}

	void node::signal_waiter()
	{
		acl::lock_guard lg(replicate_waiters_locker_);

		std::map<log_index_t, replicate_waiter_t*>::iterator
			it = replicate_waiters_.begin();


		for (; it != replicate_waiters_.end();)
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

	void node::init()
	{

	}

	bool node::do_commit()
	{

	}

	

}