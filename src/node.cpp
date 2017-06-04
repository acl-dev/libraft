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

	bool node::is_leader()
	{
		acl::lock_guard lg(metadata_locker_);
		return role_ == E_LEADER;
	}

	void node::bind_snapshot_callback(snapshot_callback *callback)
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
	node::replicate(const std::string &data, int timeout_millis)
	{
		status_t result;
		log_index_t index = 0;
		int rc = 0;
		replicate_waiter_t *waiter = new replicate_waiter_t;

		if (!write_log(data, index))
		{
			logger_fatal("write_log error");
			delete waiter;
			return{ E_UNKNOWN, { 0, 0}};
		}

		waiter->log_index_ = index;
		add_waiter(waiter);

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
			result = status_t::E_TIMEOUT;
		}
		else
		{
			result = status_t::E_UNKNOWN;
		}
		acl_pthread_mutex_unlock(waiter->mutex_);

		delete waiter;

		return{ result, {waiter->log_index_ ,current_term() } };
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

	raft::log_index_t node::last_log_index()
	{
		acl::lock_guard lg(metadata_locker_);
		return last_log_index_;
	}

	bool node::build_replicate_log_request(
		replicate_log_entries_request &requst, 
		log_index_t index ,
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

	std::vector<log_index_t> node::get_peers_match_index()
	{
		std::vector<log_index_t> indexs;
		std::map<std::string, peer*>::iterator it;

		acl::lock_guard lg(peers_locker_);

		for (it = peers_.begin(); it != peers_.end(); ++it)
		{
			indexs.push_back(it->second->match_index());
		}

		return indexs;
	}

	void node::replicate_log_callback()
	{
		std::vector<log_index_t> mactch_indexs = get_peers_match_index();

		std::sort(mactch_indexs.begin(), mactch_indexs.end());

		/*
			0/2 = 0 (1-1)/2 = 0  [1]
			1/2 = 0 (2-1)/2 = 0  [1,2]
			2/2 = 1 (3-1)/2 = 1  [1,2,3] 
			3/2 = 1 (4-1)/2 = 1  [1,2,3,4]
			4/2 = 2 (5-1)/2 = 2  [1,2,3,4,5]
			5/2 = 2 (6-1)/2 = 2  [1,2,3,4,5,6]
			6/2 = 3 (7-1)/2 = 3  [1,2,3,4,5,6,7]
		*/

		log_index_t index = mactch_indexs[mactch_indexs.size() / 2];

		update_committed_index(index);

		signal_replicate_waiter(index);
	}

	void node::build_vote_request(vote_request &req)
	{
		req.set_candidate(raft_id_);
		req.set_last_log_index(last_log_index());
		req.set_last_log_term(current_term());
	}

	void node::vote_response_callback(const std::string &peer_id, 
		const vote_response &response)
	{

	}

	void node::handle_new_term_callback(term_t term)
	{
		logger("receive new term.%d",term);

	}

	bool node::get_snapshot(std::string &path)
	{
		std::map<log_index_t, std::string> 
			snapshot_files_= scan_snapshots();

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

		if (scan.open(snapshot_path_.c_str(), false) == false)
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

	bool node::check_log_compaction()
	{
		if (log_manager_->log_count() <= max_log_count_ 
			|| check_making_snapshot())
			return false;
		return true;
	}

	bool node::check_making_snapshot()
	{
		acl::lock_guard lg(making_snapshot_locker_);
		return making_snapshot_;
	}

	void node::async_log_compaction()
	{

	}

	void node::set_making_snapshot()
	{
		acl::lock_guard lg(making_snapshot_locker_);
		making_snapshot_ = true;
	}

	void node::do_compaction_log()
	{
		bool make_snapshot_ = false;
		do 
		{
			log_index_t last_snapshot_index = 0;
			std::string filepath;

			if (get_snapshot(filepath))
			{
				acl::ifstream file;
				version ver;
				if (!file.open_read(filepath.c_str()) ||
					!read(file, ver))
				{
					logger_error("snapshot file,error,%s,",
						filepath.c_str());
				}
				else
					last_snapshot_index = ver.index_;
			}

			int dels = 0;
			std::map<log_index_t, log_index_t> log_infos
				= log_manager_->logs_info();

			acl_assert(log_infos.size());

			std::map<log_index_t, log_index_t>::iterator it;

			for (it = log_infos.begin(); it != log_infos.end(); ++it)
			{
				if (it->second < last_snapshot_index)
				{
					acl_assert(log_manager_->destroy_log(it->first));
					dels++;
					//delete half of logs
					if(dels >= log_infos.size() /2)
						break;;
				}
			}

			if (!dels && !make_snapshot_)
			{
				acl_assert(snapshot_callback_);
				acl_assert(snapshot_callback_->
					make_snapshot_callback(snapshot_path_, filepath));
				make_snapshot_ = true;
				continue;
			}

		} while (false);
		
	}

	void node::update_committed_index(log_index_t index)
	{
		acl::lock_guard lg(metadata_locker_);

		/*
			multi thread update committed_index. 
			maybe other update bigger first.
		*/
		if(committed_index_ < index)
			committed_index_ = index;
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

	void node::add_waiter(replicate_waiter_t *waiter)
	{
		acl::lock_guard lg(waiters_locker_);
		replicate_waiters_.insert(
			std::make_pair(waiter->log_index_, waiter));
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

		if (!log_manager_->write(entry))
		{
			logger_fatal("log write error");
			return false;
		}
		index = entry.index();
		if (check_log_compaction())
			async_log_compaction();
		return true;
	}

	void node::signal_replicate_waiter(log_index_t index)
	{
		acl::lock_guard lg(waiters_locker_);

		std::map<log_index_t, replicate_waiter_t*>::iterator
			it = replicate_waiters_.begin();


		for (; it != replicate_waiters_.end();)
		{
			if (it->first > index)
				break;

			replicate_waiter_t *waiter = it->second;

			acl_pthread_mutex_lock(waiter->mutex_);
			it->second->result_ = status_t::E_OK;
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
		return false;
	}

	

}