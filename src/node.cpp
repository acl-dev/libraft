#include "raft.hpp"
#include <algorithm>

#ifndef __10MB__ 
#define __10MB__ 10*1024*1024
#endif

#ifndef __10000__ 
#define __10000__ 10000
#endif

#ifndef __SNAPSHOT_EXT__
#define __SNAPSHOT_EXT__ ".snapshot"
#endif

namespace raft
{
	struct replicate_cond_t
	{
		typedef std::map<log_index_t,
			replicate_cond_t*> replicate_waiters_t;

		acl_pthread_cond_t *cond_;
		acl_pthread_mutex_t *mutex_;
		status_t result_;
		log_index_t log_index_;

		replicate_waiters_t::iterator it_;

		replicate_cond_t();
		~replicate_cond_t();
		void notify(status_t status = status_t::E_OK);
	};

	replicate_cond_t::replicate_cond_t()
	{
		acl_pthread_mutexattr_t attr;

		cond_ = acl_thread_cond_create();
		mutex_ = (acl_pthread_mutex_t*)
			acl_mymalloc(sizeof(acl_pthread_mutex_t));
		acl_pthread_mutex_init(mutex_, &attr);
	}

	replicate_cond_t::~replicate_cond_t()
	{
		acl_pthread_cond_destroy(cond_);
		acl_pthread_mutex_destroy(mutex_);
		acl_myfree(mutex_);
	}

	void replicate_cond_t::notify(status_t status)
	{
		acl_pthread_mutex_lock(mutex_);
		result_ = status;
		acl_pthread_cond_signal(cond_);
		acl_pthread_mutex_unlock(mutex_);
	}

	node::node()
		:election_timer_(*this),
		log_compaction_worker_(*this)
	{

	}

	bool node::is_leader()
	{
		acl::lock_guard lg(metadata_locker_);
		return role_ == E_LEADER;
	}

	void node::bind_snapshot_callback(
		snapshot_callback *callback)
	{
		snapshot_callback_ = callback;
	}

	void node::set_snapshot_path(const std::string &path)
	{
		snapshot_path_ = path;
	}

	void node::set_log_path(const std::string &path)
	{
		log_path_ = path;
	}

	void node::set_metadata_path(const std::string &path)
	{
		metadata_path_ = path;
	}

	void node::set_max_log_size(size_t size)
	{
		max_log_size_ = size;
	}

	void node::set_max_log_count(size_t size)
	{
		max_log_count_ = size;
	}

	std::pair<status_t, version>
		node::replicate(const std::string &data,
			unsigned int timeout_millis)
	{
		status_t result;
		log_index_t index = 0;
		term_t term = current_term();

		if (!write_log(data, index))
		{
			logger_fatal("write_log error.%s",acl::last_serror());
			return{ E_WRITE_LOG_ERROR, { 0, 0} };
		}
		
		//todo make a replicate_cond_t pool for this;
		replicate_cond_t *cond = new replicate_cond_t;

		cond->log_index_ = index;
		add_waiter(cond);
		notify_peers_replicate_log();
		acl_pthread_mutex_lock(cond->mutex_);
		if (timeout_millis)
		{
			timespec timeout;
			timeval now;

			gettimeofday(&now, NULL);
			timeout.tv_nsec = now.tv_sec;
			timeout.tv_nsec = now.tv_usec * 1000;

			timeout.tv_sec += timeout_millis / 1000;
			timeout.tv_nsec += (timeout_millis % 1000) * 1000;

			int status = acl_pthread_cond_timedwait(
				cond->cond_,
				cond->mutex_, &timeout);

			acl_pthread_mutex_unlock(cond->mutex_);

			if (status == ACL_ETIMEDOUT)
			{
				result = status_t::E_TIMEOUT;
				//timeout remove myself

				remove_waiter(cond);
			}
			else if (!status)
			{
				result = cond->result_;
			}
			else
			{
				result = status_t::E_UNKNOWN;
			}
		}
		else
		{
			acl_assert(!acl_pthread_cond_wait(cond->cond_, 
				cond->mutex_));
			result = cond->result_;
			
			acl_pthread_mutex_unlock(cond->mutex_);
		}

		delete cond;
		return{ result, { index ,term } };
	}

	std::string node::raft_id()
	{
		return raft_id_;
	}

	bool node::is_candicate()
	{
		acl::lock_guard lg(metadata_locker_);
		return role_ == E_CANDIDATE;
	}

	raft::log_index_t node::get_last_log_index()
	{
		acl::lock_guard lg(metadata_locker_);
		return last_log_index_;
	}

	raft::term_t node::current_term()
	{
		acl::lock_guard lg(metadata_locker_);
		return current_term_;
	}

	void node::update_term(term_t term)
	{
		acl::lock_guard lg(metadata_locker_);
		current_term_ = term;
	}
	node::role_t node::role()
	{
		acl::lock_guard lg(metadata_locker_);
		return role_;
	}

	void node::update_role(role_t _role)
	{
		acl::lock_guard lg(metadata_locker_);
		role_ = _role;
	}

	raft::log_index_t node::last_log_index()
	{
		acl::lock_guard lg(metadata_locker_);
		return last_log_index_;
	}

	bool node::build_replicate_log_request(
		replicate_log_entries_request &requst,
		log_index_t index,
		int entry_size)
	{
		requst.set_leader_id(raft_id_);
		requst.set_leader_commit(committed_index_);

		if (!entry_size)
			entry_size = __10000__;

		//log empty 
		if (last_log_index() == 0)
		{
			requst.set_prev_log_index(0);
			requst.set_prev_log_term(0);
			return true;
		}
		else if (index <= last_log_index())
		{
			std::vector<log_entry> entries;
			//index -1 for prev_log_term, set_prev_log_index
			if (log_manager_->read(index - 1,
				__10MB__,
				entry_size,
				entries))
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
			/*peer match leader now .and just make heartbeat req*/
			log_entry entry;
			/*index -1 for prev_log_term, prev_log_index */
			if (log_manager_->read(index - 1, entry))
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

	std::vector<log_index_t> 
		node::get_peers_match_index()
	{
		std::vector<log_index_t> indexs;
		std::map<std::string, peer*>::iterator it;

		acl::lock_guard lg(peers_locker_);

		for (it = peers_.begin(); 
			it != peers_.end(); ++it)
		{
			indexs.push_back(it->second->match_index());
		}

		return indexs;
	}

	void node::replicate_log_callback()
	{
		std::vector<log_index_t> mactch_indexs 
			= get_peers_match_index();

		//myself 
		mactch_indexs.push_back(last_log_index());

		std::sort(mactch_indexs.begin(), 
			mactch_indexs.end());

		log_index_t majority_index = 
			mactch_indexs[mactch_indexs.size() / 2];

		update_committed_index(majority_index);

		signal_replicate_waiter(majority_index);
	}

	void node::build_vote_request(vote_request &req)
	{
		req.set_candidate(raft_id_);
		req.set_last_log_index(last_log_index());
		req.set_last_log_term(current_term());
	}

	int node::peers_count()
	{
		acl::lock_guard lg(peers_locker_);
		return (int)peers_.size();
	}

	void node::clear_vote_response()
	{
		acl::lock_guard lg(vote_responses_locker_);
		vote_responses_.clear();
	}

	void node::vote_response_callback(
		const std::string &peer_id,
		const vote_response &response)
	{
		if (response.term() < current_term())
		{
			logger("handle vote_response, but term is old");
			return;
		}

		if (role() != role_t::E_CANDIDATE)
		{
			logger("handle vote_response, but not candidate");
			return;
		}

		int peers = peers_count();
		int votes = 1;//myself

		vote_responses_locker_.lock();
		vote_responses_[peer_id] = response;

		std::map<std::string, vote_response>::iterator it;
		for (it = vote_responses_.begin()
			; it != vote_responses_.end();++it)
		{
			if (it->second.vote_granted())
				votes++;
		}
		vote_responses_locker_.unlock();

		//majority. +1 for myself
		if (votes > (peers + 1) / 2)
		{
			become_leader();
		}
	}

	void node::become_leader()
	{
		update_role(role_t::E_LEADER);

		update_peers_next_index();

		notify_peers_replicate_log();


	}

	void node::handle_new_term_callback(term_t term)
	{
		logger("receive new term.%d", term);

	}

	bool node::get_snapshot(std::string &path)
	{
		std::map<log_index_t, std::string>
			snapshot_files_ = scan_snapshots();

		if (snapshot_files_.empty())
			return false;

		path = snapshot_files_.rbegin()->second;
		return true;
	}

	std::map<log_index_t, std::string>
		node::scan_snapshots()
	{

		acl::scan_dir scan;
		const char* filepath = NULL;
		std::map<log_index_t, std::string> snapshots;

		if (scan.open(snapshot_path_.c_str(),false) == false) 
		{
			logger_error("scan open error %s\r\n",
				acl::last_serror());
			return;
		}

		while ((filepath = scan.next_file(true)) != NULL)
		{
			if (acl_strrncasecmp(filepath,
				__SNAPSHOT_EXT__,
				strlen(__SNAPSHOT_EXT__)) == 0)
			{
				version ver;
				acl::ifstream file;

				if (!file.open_read(filepath))
				{
					logger_error("open file error.%s",
						acl::last_serror());
					continue;
				}

				if (!read(file, ver))
				{
					logger_error("read_version file.%s",
						filepath);
					file.close();
					continue;
				}
				file.close();
				snapshots[ver.index_] = filepath;
			}
		}
		return  snapshots;
	}

	bool node::should_compact_log()
	{
		if (log_manager_->log_count() <= max_log_count_ || 
			check_compacting_log())
			return false;

		return true;
	}

	bool node::check_compacting_log()
	{
		acl::lock_guard lg(compacting_log_locker_);
		return compacting_log_;
	}

	void node::async_compact_log()
	{
		acl::lock_guard lg(compacting_log_locker_);

		//check again.
		if (!compacting_log_)
		{
			compacting_log_ = true;
			log_compaction_worker_.compact_log();
		}
	}

	bool node::make_snapshot()
	{
		std::string filepath;
		if (snapshot_callback_->make_snapshot_callback(
			snapshot_path_,
			filepath))
		{
			logger("make_snapshot ok.%s",filepath.c_str());
			return true;
		}
		return false;
	}
	void node::do_compaction_log()
	{
		bool		try_make_snapshot = false;

	try_again:
		log_index_t last_snapshot_index = 0;
		std::string filepath;
		int			dels = 0;

		if (get_snapshot(filepath))
		{
			acl::ifstream	file;
			version			ver;

			if (!file.open_read(filepath.c_str()) ||
				!read(file, ver))
			{
				logger_error("snapshot file,error,%s,",
					filepath.c_str());
			}
			else
				last_snapshot_index = ver.index_;
		}

		std::map<log_index_t, log_index_t> log_infos = 
			log_manager_->logs_info();

		std::map<log_index_t, log_index_t>::iterator it;

		acl_assert(log_infos.size());

		for (it = log_infos.begin();it != log_infos.end();++it)
		{
			if (it->second < last_snapshot_index)
			{
				acl_assert(log_manager_->
					destroy_log(it->first));
				
				dels++;
				//delete half of logs
				if (dels >= log_infos.size() / 2)
					break;;
			}
		}

		if (!dels && !try_make_snapshot)
		{
			try_make_snapshot = true;
			if(make_snapshot())
				goto try_again;
		}

	}

	void node::update_committed_index(log_index_t index)
	{
		acl::lock_guard lg(metadata_locker_);

		/*multi thread update committed_index.
			maybe other update bigger first.*/
		if (committed_index_ < index)
			committed_index_ = index;
	}

	void node::set_election_timer()
	{
		unsigned int timeout = election_timeout_;

		srand(time(NULL));
		timeout += rand() % timeout;

		election_timer_.set_timer(timeout);
		logger("set_election_timer, "
			"%d milliseconds later", timeout);
	}

	void node::cancel_election_timer()
	{
		election_timer_.cancel_timer();
	}

	void node::election_timer_callback()
	{
		if (role() != role_t::E_CANDIDATE)
		{
			cancel_election_timer();
			return;
		}
		clear_vote_response();
		update_term(current_term() + 1);
		notify_peers_to_election();
		set_election_timer();
	}

	raft::log_index_t node::committed_index()
	{
		acl::lock_guard lg(metadata_locker_);
		return committed_index_;
	}

	raft::log_index_t node::start_log_index()
	{
		return log_manager_->start_log_index();
	}

	void node::notify_peers_to_election()
	{
		acl::lock_guard lg(peers_locker_);

		std::map<std::string, peer *>::iterator it;
		
		for (it = peers_.begin();
			it != peers_.end(); ++it)
		{
			it->second->notify_election();
		}
	}

	void  node::update_peers_next_index()
	{
		log_index_t index = last_log_index();

		acl::lock_guard lg(peers_locker_);

		std::map<std::string, peer *>::iterator it =
			peers_.begin();
		for (; it != peers_.end(); ++it)
		{
			it->second->set_next_index(index);
		}
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
		return false;
	}

	bool node::handle_install_snapshot_requst(
		const install_snapshot_request &req,
		install_snapshot_response &resp)
	{
		return false;
	}

	void node::add_waiter(replicate_cond_t *waiter)
	{
		acl::lock_guard lg(waiters_locker_);

		acl_pthread_mutex_lock(waiter->mutex_);
		
		waiter->it_ = replicate_waiters_.insert(
			std::make_pair(waiter->log_index_, waiter)).first;
		
		acl_pthread_mutex_unlock(waiter->mutex_);
	}
	void node::remove_waiter(replicate_cond_t *waiter)
	{
		acl::lock_guard lg(waiters_locker_);

		acl_pthread_mutex_lock(waiter->mutex_);

		if(waiter->it_ != replicate_waiters_.end())
			replicate_waiters_.erase(waiter->it_);
		waiter->it_ = replicate_waiters_.end();

		acl_pthread_mutex_unlock(waiter->mutex_);
	}
	log_index_t node::gen_log_index()
	{
		acl::lock_guard lg(metadata_locker_);
		return ++last_log_index_;
	}
	void node::make_log_entry(const std::string &data, log_entry &entry)
	{
		log_index_t index;
		term_t term = current_term();

		entry.set_index(gen_log_index());
		entry.set_term(term);
		entry.set_log_data(data);
		entry.set_type(log_entry_type::e_raft_log);
	}

	bool node::write_log(const std::string &data, log_index_t &index)
	{

		log_entry entry;

		make_log_entry(data, entry);
		if (!log_manager_->write(entry))
		{
			logger_fatal("log write error");
			return false;
		}
		index = entry.index();

		if (should_compact_log())
			async_compact_log();
		return true;
	}

	void node::signal_replicate_waiter(log_index_t index)
	{
		acl::lock_guard lg(waiters_locker_);

		std::map<log_index_t, replicate_cond_t*>::iterator
			it = replicate_waiters_.begin();


		for (; it != replicate_waiters_.end();)
		{
			if (it->first > index)
				break;

			replicate_cond_t *waiter = it->second;
			waiter->notify();
			it = replicate_waiters_.erase(it);
		}
	}

	void node::init()
	{

	}

	bool node::do_commit()
	{
		return false;
	}



}