#include "raft.hpp"
#include <algorithm>
#include <iostream>

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
	const static std::string g_magic_string("raft-snapshot-head");

	struct snapshot_head
	{
		std::string magic_string_;
		snapshot_info info_;
	};

	bool write(acl::ostream &stream, const version &ver)
	{
		snapshot_head head;

		head.magic_string_ = g_magic_string;
		head.info_.set_last_included_term(ver.term_);
		head.info_.set_last_snapshot_index(ver.index_);

		if (!write(stream, head.magic_string_))
			return false;

		std::string buffer = head.info_.SerializeAsString();
        return write(stream, buffer);

    }

	bool read(acl::istream &file, version &ver)
	{
		std::string magic_string;
		std::string buffer;
		snapshot_info info;

		if (!read(file, magic_string) || magic_string != g_magic_string)
		{
			logger_error("read snapshot error,magic_string:%s",
				magic_string.c_str());
			return false;
		}

		if (!read(file, buffer) || !info.ParseFromString(buffer))
		{
			logger_error("read snapshot error");
			return false;
		}

		ver.index_ = info.last_snapshot_index();
		ver.term_ = info.last_included_term();
		return true;
	}

	bool operator<(const version& left, const version& right)
	{
		return left.index_ < right.index_ || 
			left.term_ < right.term_;
	}

	node::node()
		: log_manager_(NULL),
		election_timeout_(3000),
		committed_index_(0),
		applied_index_(0),
		current_term_(0),
		role_(E_FOLLOWER),
		load_snapshot_callback_(NULL),
		make_snapshot_callback_(NULL),
		snapshot_info_(NULL),
		snapshot_tmp_(NULL),
		last_snapshot_index_(0),
		last_snapshot_term_(0),
		max_log_size_(4 * 1024 * 1024),
		max_log_count_(5),
		compacting_log_(false),
		election_timer_(*this),
		log_compaction_worker_(*this),
		apply_callback_(NULL),
		apply_log_(*this)
	{
	}

	node::~node()
	{
		acl::lock_guard lg(peers_locker_);
		std::map<std::string, peer *>::iterator it = peers_.begin();
		for(; it != peers_.end(); ++it)
		{
			delete it->second;
		}
	}

	bool node::replicate(const std::string& data, replicate_callback* callback)
	{
		term_t		term = 0;
		log_index_t index = 0;

		if (!is_leader())
		{
			logger("node is not leader .is %s",
				role() == E_FOLLOWER ? "follower" : "candidate");
			return false;
		}
		if (!write_log(data, index, term))
		{
			logger_error("write_log error.%s", acl::last_serror());
			return false;
		}

		add_replicate_callback(version(index, term), callback);

		notify_peers_replicate_log();

		return true;
	}

	bool node::read(log_index_t index, std::string& data)
	{
		log_entry log;
		log_index_t _applied_index = applied_index();
		if (_applied_index < index)
		{
			logger_error("index error.index must not "
				"> applied index."
				"index:%llu ."
				"applied_index:%llu",
				index, 
				_applied_index);
			return false;
		}
			
		if(log_manager_->read(index, log))
		{
			data = log.log_data();
			return true;
		}
		logger_error("read log error");
		return false;
	}

	bool node::is_leader()
	{
		acl::lock_guard lg(metadata_locker_);
		return role_ == E_LEADER;
	}

	void node::set_load_snapshot_callback(
		load_snapshot_callback *callback)
	{
		load_snapshot_callback_ = callback;
	}
	void node::set_apply_callback(apply_callback *callback)
	{
		apply_callback_ = callback;
	}
	void node::set_snapshot_path(const std::string &path)
	{
		acl::lock_guard lg(metadata_locker_);
		snapshot_path_ = path;

		if (snapshot_path_.size())
		{
			//back
			char ch = snapshot_path_[snapshot_path_.size() - 1];
			if (ch != '/'  && ch != '\\')
			{
				snapshot_path_.push_back('/');
			}
		}
        load_last_snapshot_info();
	}

	void node::set_log_path(const std::string &path)
	{
		log_path_ = path;
		acl_assert(!log_manager_);
		log_manager_ = new mmap_log_manager(log_path_);
		log_manager_->set_log_size(max_log_size_);
	}

	void node::set_metadata_path(const std::string &path)
	{
		metadata_path_ = path;
	}

	void node::set_max_log_size(size_t size)
	{
		max_log_size_ = size;
		if(log_manager_)
			log_manager_->set_log_size(max_log_size_);
	}

	void node::set_max_log_count(size_t size)
	{
		max_log_count_ = size;
	}
	void node::load_last_snapshot_info()
	{
		std::string file_path;
		if (get_snapshot(file_path))
		{
			version ver;
			acl::ifstream file;
			if (!file.open_read(file_path.c_str()))
			{
				logger_fatal("open_read error."
					"file path%s", 
					file_path.c_str());
				return;
			}
			if (!raft::read(file, ver))
			{
				logger_fatal("read version error");
				return;
			}
			set_last_snapshot_index(ver.index_);
			set_last_snapshot_term(ver.term_);
		}
	}

	std::string node::raft_id()const
	{
		return raft_id_;
	}

	bool node::is_candidate()
	{
		acl::lock_guard lg(metadata_locker_);
		return role_ == E_CANDIDATE;
	}

	raft::term_t node::current_term()
	{
		acl::lock_guard lg(metadata_locker_);
		return current_term_;
	}
	std::string node::leader_id()
	{
		acl::lock_guard lg(metadata_locker_);
		return leader_id_;
	}

	void node::set_peers(const std::vector<std::string> &peers)
	{
		acl::lock_guard lg(peers_locker_);

		for(size_t i = 0; i < peers.size(); ++i)
		{
			if(peers_.find(peers[i]) == peers_.end())
			{
				peer *_peer = new peer(*this, peers[i]);
				peers_.insert(std::make_pair(peers[i], _peer));
			}
		}
	}

	void node::set_make_snapshot_callback(
		make_snapshot_callback* callback)
	{
		make_snapshot_callback_ = callback;
	}

	void node::set_leader_id(const std::string &leader_id)
	{
		acl::lock_guard lg(metadata_locker_);
		if (leader_id_ != leader_id)
			logger("find new leader.%s", leader_id.c_str());
		leader_id_ = leader_id;
	}
	void node::set_current_term(term_t term)
	{
        logger_debug(1,2,"set term to %llu",term);

		acl::lock_guard lg(metadata_locker_);

		if (current_term_ < term)
			/*new term. has a vote to election who is leader*/
			vote_for_.clear();
		current_term_ = term;
	}

	int node::role()
	{
		acl::lock_guard lg(metadata_locker_);
		return role_;
	}

	void node::set_vote_for(const std::string& vote_for)
	{
		acl::lock_guard lg(metadata_locker_);
		vote_for_ = vote_for;
	}

	std::string node::vote_for()
	{
		acl::lock_guard lg(metadata_locker_);
		return vote_for_;
	}

	void node::set_role(int _role)
	{
        logger_debug(1,2,"set role to %s",
                     _role == E_CANDIDATE ? "candidate":
                     (_role == E_FOLLOWER ? "follower":"leader"));

		acl::lock_guard lg(metadata_locker_);
		role_ = _role;
	}

	log_index_t node::applied_index()
	{
		acl::lock_guard lg(metadata_locker_);
		return applied_index_;
	}

	void node::set_applied_index(log_index_t index)
	{
		acl::lock_guard lg(metadata_locker_);
		if (index != applied_index_ + 1)
		{
			logger_fatal("apply index error");
			return;
		}
		applied_index_ = index;
	}
    raft::term_t node::last_log_term()const
    {
        return log_manager_->last_term();
    }
	raft::log_index_t node::last_log_index() const
	{
		return log_manager_->last_index();
	}

	bool node::build_replicate_log_request(
		replicate_log_entries_request &request,
		log_index_t index,
		int entry_size)
	{
        request.set_term(current_term());
		request.set_leader_id(raft_id_);
		request.set_leader_commit(committed_index_);

		if (!entry_size)
			entry_size = __10000__;

		//log empty 
		if (last_log_index() == 0)
		{
			request.set_prev_log_index(0);
			request.set_prev_log_term(0);
			return true;
		}
		else if (index <= last_log_index())
		{
			std::vector<log_entry> entries;
			//index -1 for prev_log_term, set_prev_log_index
			if (log_manager_->read(index - 1, __10MB__,
				entry_size, entries))
			{
				request.set_prev_log_index(entries[0].index());
				request.set_prev_log_term(entries[0].index());
				//first one is prev log
				for (size_t i = 1; i < entries.size(); i++)
				{
					//copy
					*request.add_entries() = entries[i];
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
				request.set_prev_log_index(entry.index());
				request.set_prev_log_term(entry.index());

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

		for (it = peers_.begin();
			it != peers_.end(); ++it)
		{
			indexs.push_back(it->second->match_index());
		}

		return indexs;
	}

	void node::replicate_log_callback()
	{
		/*
		 * If there exists an N such that N > commitIndex, a majority
		 * of matchIndex[i] �� N, and log[N].term == currentTerm:
		 * set commitIndex = N (��5.3, ��5.4).
		*/
		if (!is_leader())
		{
			logger("not leader.");
			return;
		}

		std::vector<log_index_t>
			mactch_indexs = get_peers_match_index();

		mactch_indexs.push_back(last_log_index());//myself 

		std::sort(mactch_indexs.begin(), mactch_indexs.end());

		log_index_t majority_index
			= mactch_indexs[mactch_indexs.size() / 2];

		if (majority_index > committed_index())
		{
			set_committed_index(majority_index);
			apply_log_.do_apply();
		}
	}

	void node::build_vote_request(vote_request &req)
	{

		req.set_candidate(raft_id_);
		req.set_last_log_index(last_log_index());
		req.set_last_log_term(last_log_term());
        req.set_term(current_term());
        logger_debug(2, 2, "req.term = %lu", req.term());
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
			logger("handle vote_response, but term is old. "
                   "current_term:%llu,"
                   "response.term:%lu",
                   current_term(),
                   response.term());
			return;
		}

		if (role() != E_CANDIDATE)
		{
			logger("handle vote_response, but not candidate");
			return;
		}

		int nodes = peers_count() + 1;//+1 for myself
		int votes = 1;//myself

		vote_responses_locker_.lock();

		vote_responses_[peer_id] = response;
		std::map<std::string, vote_response>
			::iterator it = vote_responses_.begin();

		for (; it != vote_responses_.end(); ++it)
		{
			if (it->second.vote_granted())
			{
				votes++;
			}
		}
		vote_responses_locker_.unlock();

		/*
		 * If votes received from majority of servers:
		 * become leader
		 */
        logger_debug(1,2,"votes:%d", votes);

		if (votes > nodes / 2)
		{
			become_leader();
		}
	}

	void node::become_leader()
	{
        logger_debug(1,2,"trace");

		cancel_election_timer();

		set_role(E_LEADER);

		clear_vote_response();
		/*
		 * Reinitialized after election
		 * nextIndex[] for each server, index of the next log entry
		 * to send to that server (initialized to leader last log index + 1)
		 * matchIndex[] for each server, index of highest log entry
		 * known to be replicated on server
		 * (initialized to 0, increases monotonically)
		 */
		update_peers_next_index(last_log_index() + 1);

		update_peers_match_index(0);

		notify_peers_replicate_log();
	}

	void node::handle_new_term(term_t term)
	{
		logger("receive new term.%llu", term);
		set_current_term(term);
		step_down();
	}

	bool node::get_snapshot(std::string &path) const
	{
		std::map<log_index_t, std::string>
			snapshot_files_ = scan_snapshots();

		if (snapshot_files_.size())
		{
			path = snapshot_files_.rbegin()->second;
			return true;
		}
		return false;
	}

	std::map<log_index_t, std::string>
		node::scan_snapshots() const
	{

		acl::scan_dir scan;
		const char* filepath = NULL;
		std::map<log_index_t, std::string> snapshots;

		if (!scan.open(snapshot_path_.c_str(), false))
		{
			logger_error("scan open error %s\r\n",
				acl::last_serror());
			return snapshots;
		}

		while ((filepath = scan.next_file(true)) != NULL)
		{
			if (acl_strrncasecmp(filepath, __SNAPSHOT_EXT__,
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
				if (!raft::read(file, ver))
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
        if (log_manager_->log_count() <= max_log_count_)
        {
            return false;
        }
        else
        {
            if (check_compacting_log())
            {
                return false;
            }
        }
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
			log_compaction_worker_.do_compact_log();
		}
	}

	bool node::make_snapshot() const
	{
		std::string filepath;
		acl_assert(make_snapshot_callback_);

		if (!(*make_snapshot_callback_)(snapshot_path_, filepath))
		{
			logger_error("make_snapshot error.path:%s",
				snapshot_path_.c_str());
			return false;
		}

		std::string snapshot_file = filepath;
		size_t pos = filepath.find_last_of('.');
		if (pos != filepath.npos)
		{
			snapshot_file = filepath.substr(0, pos);
		}
		else
		{
			snapshot_file += __SNAPSHOT_EXT__;
		}

		if (rename(filepath.c_str(), snapshot_file.c_str()) != 0)
		{
			logger_error("rename failed."
				"last error:%s",
				acl::last_serror());
			
		}
		else
		{
			logger("make_snapshot done."
				"file path:%s", 
				snapshot_file.c_str());

			return true;
		}


		return false;
	}
	void node::do_compaction_log() const
	{

	do_again:
		std::string snapshot;
        if (!get_snapshot(snapshot))
        {
			if (make_snapshot())
			{
				goto do_again;
			}
			else
			{
				logger_error("make_snapshot failed.");
				return;
			}
		}

		acl::ifstream	file;
		if (!file.open_read(snapshot.c_str()))
		{
			logger_error("open file,error,%s", 
				snapshot.c_str());
			return;
		}

		version ver;
		if (!raft::read(file, ver))
		{
			logger_error("read vesion error");
			return;
		}
			
		int	count = 0;
		log_infos_t log_infos = log_manager_->logs_info();
		log_infos_iter_t it = log_infos.begin();

		for (; it != log_infos.end(); ++it)
		{
			if (it->second <= ver.index_)
			{
				count += log_manager_->discard_log(it->second);
			}
			//delete half of logs
			if (count >= log_infos.size() / 2)
			{
				break;
			}
		}
		if (!count)
		{
			if (make_snapshot())
			{
				goto do_again;
			}
		}
		logger("log_compaction discard %d logs",count);
	}

	void node::set_committed_index(log_index_t index)
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

		srand(static_cast<unsigned int>(time(NULL) % 0xffffffff));
		timeout += rand() % timeout;

		election_timer_.set_timer(timeout);
		logger("set_election_timer, "
               "%d milliseconds later",
               timeout);
	}

	void node::cancel_election_timer()
	{
		election_timer_.cancel_timer();
	}

	void node::election_timer_callback()
	{

        logger("election timer callback");
		/*
		 * this node lost heartbeat from leader
		 * and it has not leader now.so this node 
		 * should elect to be new leader
		 */
		set_leader_id("");

		/*Rule For Followers:
		* If election timeout elapses without receiving AppendEntries
		* RPC from current leader or granting vote to candidate:
		* convert to candidate
		*/
		if (role() == E_FOLLOWER && vote_for().size())
		{
			set_election_timer();
            logger("vote_for is not empty. return");
			return;
		}


		/*
		* : conversion to candidate, start election:
		* : Increment currentTerm
		* : Vote for self
		* : Reset election timer
		* : Send RequestVote RPCs to all other servers
		* : If election timeout elapses: start new election
		*/

		clear_vote_response();

		set_role(E_CANDIDATE);

        set_current_term(current_term() + 1);

        set_vote_for(raft_id_);

        notify_peers_to_election();

        set_election_timer();
	}

	raft::log_index_t node::committed_index()
	{
		acl::lock_guard lg(metadata_locker_);
		return committed_index_;
	}

	raft::log_index_t node::start_log_index()const
	{
		return log_manager_->start_index();
	}

	void node::notify_peers_to_election()
	{
        logger_debug(1,2,"trace");

		acl::lock_guard lg(peers_locker_);

		std::map<std::string, peer *>::iterator it = peers_.begin();
		for (; it != peers_.end(); ++it)
		{
			it->second->notify_election();
		}
	}

	void  node::update_peers_next_index(log_index_t index)
	{
		acl::lock_guard lg(peers_locker_);

		std::map<std::string, peer *>::iterator it = peers_.begin();
		for (; it != peers_.end(); ++it)
		{
			it->second->set_next_index(index);
		}
	}

	void node::update_peers_match_index(log_index_t index)
	{
		acl::lock_guard lg(peers_locker_);

		std::map<std::string, peer *>::iterator it = peers_.begin();
		for (; it != peers_.end(); ++it)
		{
			it->second->set_match_index(index);
		}
	}

	void node::notify_peers_replicate_log()
	{
		acl::lock_guard lg(peers_locker_);
		std::map<std::string, peer *>::iterator it = peers_.begin();
		for (; it != peers_.end(); ++it)
		{
            it->second->notify_replicate();
		}
	}

	bool node::handle_vote_request(const vote_request &req,
		vote_response &resp)
	{
		resp.set_req_id(req.req_id());
		resp.set_term(current_term());
		resp.set_log_ok(false);

		/* Reply false if term < currentTerm (5.1)*/
		if (req.term() < current_term())
		{
			resp.set_vote_granted(false);
            logger_debug(2,1,
                         "req.term(%lu) "
                         "current_term(%llu)",
                         req.term(),
                         current_term());
			return true;
		}
		/*
		 * If votedFor is null or candidateId, and candidate's log is at
		 * least as up-to-date as receiver's log, grant vote (��5.2, ��5.4)
		 */
		if (req.last_log_index() > last_log_index())
		{
			resp.set_log_ok(true);
		}
		else if (req.last_log_index() == last_log_index())
		{
			if (req.last_log_term() == log_manager_->last_term())
			{
				resp.set_log_ok(true);
			}
		}

		if (req.term() > current_term())
		{
			/*step down to follower then discover 
			new node with higher term*/

			step_down();
			set_current_term(req.term());
		}

		if (req.term() == current_term())
		{
			if (resp.log_ok() && vote_for().empty())
			{
				set_vote_for(req.candidate());
				resp.set_vote_granted(true);
			}
		}
		resp.set_term(current_term());
		return true;
	}

	void node::invoke_apply_callbacks()
	{
		log_index_t committed = committed_index();

		for (log_index_t index = applied_index() + 1; 
			index <= committed; ++index)
		{
			log_entry entry;
			version ver;
			if (log_manager_->read(index, entry))
			{
				ver.index_ = entry.index();
				ver.term_ = entry.term();
				if (!(*apply_callback_)(entry.log_data(), ver))
				{
					logger_error("apply_callback::operator() error");
					return;
				}
				set_applied_index(index);
				continue;
			}
			logger_error("read log error");
			return;
		}
	}

    void node::notify_replicate_failed()
    {
        logger_debug(1, 2, "trace");

        acl::lock_guard lg(replicate_callbacks_locker_);
        replicate_callbacks_t::iterator it = replicate_callbacks_.begin();
        for (; it != replicate_callbacks_.end();)
        {
            if (!(*(it->second))(replicate_callback::E_NO_LEADER,
                                 it->first))
            {
                return;
            }
        }
    }
	void node::invoke_replicate_callback(replicate_callback::status_t status)
	{
		log_index_t committed = committed_index();

		acl::lock_guard lg(replicate_callbacks_locker_);

		replicate_callbacks_t::iterator it = replicate_callbacks_.begin();
		for (; it != replicate_callbacks_.end();)
		{
			if (it->first.index_ <= committed)
			{
				if (!(*(it->second))(status, it->first))
				{
					logger_error("replicate_callback::operator()() .error");
					return;
				}
				set_applied_index(it->first.index_);
				replicate_callbacks_.erase(it++);
				continue;
			}
			break;
		}
	}

	bool node::handle_replicate_log_request(
		const replicate_log_entries_request &req,
		replicate_log_entries_response &resp)
	{
        logger_debug(1,2,"-------handle_replicate_log_request------------");

		resp.set_req_id(req.req_id());
		/*currentTerm, for leader to update itself*/
		resp.set_term(current_term());
		resp.set_last_log_index(last_log_index());

		/*Reply false if term < currentTerm (5.1)*/
		if (req.term() < current_term())
		{
            logger_debug(1,2,"req.term(%lu) < current_term(%llu)",
                         req.term(),
                         current_term());

			resp.set_success(false);
			return true;
		}


		/*
		 *If RPC request or response contains term T > currentTerm:
		 *set currentTerm = T, convert to follower (5.1)
		 */
		set_current_term(req.term());
		step_down();
		set_leader_id(req.leader_id());

		resp.set_term(current_term());

		/*Reply false if log doesn't contain an entry at prevLogIndex
		 *whose term matches prevLogTerm (5.3)
		 */
		if (req.prev_log_index() > last_log_index())
		{
			resp.set_success(false);
			return true;
		}
		else if (req.prev_log_index() == last_log_index())
		{

			if (req.prev_log_index() == last_snapshot_index())
			{
				if (req.prev_log_term() != last_snapshot_term())
				{
					logger_fatal("cluster error.....");
					return true;
				}
			}
			else
			{
				/*
				* reply false if log does not contain an entry at prevLogIndex
				* whose term matches prevLogTerm (5.3)
				*/
				if (req.prev_log_term() != log_manager_->last_term())
				{
					resp.set_last_log_index(req.prev_log_index() - 1);
				}
			}
		}
		else if (req.prev_log_index() >= start_log_index())
		{
			log_entry entry;

			if (log_manager_->read(req.prev_log_index(), entry))
			{
				/*
				* check log_entry sync.
				*/
				if (req.prev_log_term() != entry.term())
				{
					resp.set_last_log_index(req.prev_log_index() - 1);
					return true;
				}
			}
			else
			{
				logger_fatal("read log error.");
				return true;
			}
		}
		else
		{
			resp.set_last_log_index(last_snapshot_index());
			return true;
		}

		resp.set_success(true);

		bool sync_log = true;

		for (int i = 0; i < req.entries_size(); i++)
		{
			const log_entry &entry = req.entries(i);
			if (sync_log && entry.index() <= last_log_index())
			{
				log_entry tmp;
				if (log_manager_->read(entry.index(), tmp))
				{
					if (entry.term() == tmp.term())
						continue;
					/*
					 *  If an existing entry conflicts with a new one
					 *  (same index but different terms), delete the
					 *  existing entry and all that follow it (��5.3)
					 */
					log_manager_->truncate(entry.index());
					sync_log = false;
				}
			}
			/* Append any new entries not already in the log */
			if (!log_manager_->write(entry))
			{
				logger_error("write log error...");
				return true;
			}
		}
		/*
		 *  If leaderCommit > commitIndex,
		 *  set commitIndex = min(leaderCommit, index of last new entry)
		 */
		if (req.leader_commit() > committed_index())
		{
			log_index_t index = req.leader_commit();
			if (index > last_log_index())
			{
				index = last_log_index();
			}
			set_committed_index(index);
			apply_log_.do_apply();
		}

		return true;
	}

	void node::close_snapshot()
	{
		acl_assert(snapshot_info_);
		acl_assert(snapshot_tmp_);

		delete snapshot_info_;
		snapshot_info_ = NULL;

		snapshot_tmp_->close();
		delete snapshot_tmp_;
		snapshot_tmp_ = NULL;
	}

	acl::fstream* node::get_snapshot_tmp(const snapshot_info &info)
	{
		if (!snapshot_info_)
		{
			/*
			 *Create new snapshot file if first chunk (offset is 0)
			 */
			acl::string file_path = snapshot_path_.c_str();
			file_path.format_append("%lu.snapshot_tmp",
				info.last_snapshot_index());
			snapshot_info_ = new snapshot_info(info);
			acl_assert(!snapshot_tmp_);
			snapshot_tmp_ = new acl::fstream();
			if (!snapshot_tmp_->open_trunc(file_path))
			{
				logger_error("open filename error,file_path:%s,%s",
					file_path.c_str(),
					acl::last_serror());
				return NULL;
			}
			return snapshot_tmp_;
		}
		if (info != *snapshot_info_)
		{
			const char *filepath = snapshot_tmp_->file_path();

			logger_error("snapshot_info not match current snapshot temp file."
				"remove old snapshot file. %s", filepath);

			close_snapshot();
			remove(filepath);
			return get_snapshot_tmp(info);
		}
		return snapshot_tmp_;
	}

	void node::step_down()
	{
        logger_debug(1,2,"trace");

		if (role() == E_LEADER)
		{
            notify_replicate_failed();
		}
		else if (role() == E_CANDIDATE)
		{
			clear_vote_response();
		}
		set_role(E_FOLLOWER);
		set_election_timer();
	}

	void node::load_snapshot_file()
	{
		version ver;

		acl_assert(snapshot_tmp_);

		std::string file_path = snapshot_tmp_->file_path();

		acl_assert(snapshot_tmp_->fseek(0, SEEK_SET) != -1);
		if (!raft::read(*snapshot_tmp_, ver))
		{
			logger_error("read snapshot file error.path :%s",
				snapshot_tmp_->file_path());
			close_snapshot();
			remove(file_path.c_str());
			return;
		}
		close_snapshot();

		std::string temp;
		if (get_snapshot(temp))
		{
			acl::ifstream file;
			version temp_ver;

			acl_assert(file.open_read(temp.c_str()));
			acl_assert(raft::read(file, temp_ver));
			file.close();

			if (ver < temp_ver)
			{
				logger("snapshot_tmp(%s) is old",
					file_path.c_str());
				return;
			}
		}

		std::string snapshot = file_path;
		size_t pos = snapshot.find_last_of('.');
		if(pos != snapshot.npos)
		{
			snapshot = snapshot.substr(0, pos);
		}
		snapshot += __SNAPSHOT_EXT__;

		/*save snapshot file*/
		if (rename(file_path.c_str(), snapshot.c_str()) != 0)
		{
			logger_error("rename error."
				"oldFilePath:%s, "
				"newFilePath:%s, "
				"error:%s",
				file_path.c_str(),
				snapshot.c_str(), 
				acl::last_serror());
		}

		/*it must be*/
		if (last_log_index() < ver.index_)
		{
			/*discard the entire log*/
			int count = log_manager_->discard_log(ver.index_);
			log_manager_->set_last_index(ver.index_);
			log_manager_->set_last_term(ver.term_);
			logger("log_manager delete %d log files", count);
		}
		else
		{
			logger_fatal("error snapshot.something error happened");
			return;
		}

		set_last_snapshot_index(ver.index_);
		set_last_snapshot_term(ver.term_);

		acl_assert(load_snapshot_callback_);
		/*
		 *  Reset state machine using snapshot contents
		 *  (and load snapshot��s cluster configuration)
		 */
		if (!(*load_snapshot_callback_)(snapshot))
		{
			logger_error("receive_snapshot_callback "
				"failed,filepath:%s ",
				file_path.c_str());
			return;
		}
		logger("load_snapshot_file ok ."
			"file_path:%s,"
			"last_log_index:%llu,"
			"last_log_term:%llu,",
			snapshot.c_str(), ver.index_, ver.term_);

		acl::lock_guard lg(metadata_locker_);
		applied_index_ = ver.index_;
		committed_index_ = ver.index_;

		/* discard any existing or partial snapshot with a smaller index*/

	}
    void node::start()
    {
        set_election_timer();
    }

	bool node::handle_install_snapshot_request(
            const install_snapshot_request &req,
            install_snapshot_response &resp)
	{
		acl::fstream *file = NULL;

		resp.set_req_id(resp.req_id());

		if (req.term() < current_term())
		{
			resp.set_bytes_stored(0);
			resp.set_term(current_term());
			return true;
		}

		step_down();
		set_current_term(req.term());
		set_leader_id(req.leader_id());

		acl::lock_guard lg(snapshot_locker_);
		acl_assert(file = get_snapshot_tmp(req.snapshot_info()));
		if (file->fsize() != req.offset())
		{
			logger("offset error");
			resp.set_bytes_stored(file->fsize());
			return true;
		}
		/* Write data into snapshot file at given offset*/
		const std::string &data = req.data();
		if (file->write(data.c_str(), data.size()) != data.size())
			logger_fatal("file write error.%s", acl::last_serror());

		resp.set_bytes_stored(file->fsize());

		/*. Reply and wait for more data chunks if done is false*/
		if (req.done())
		{
			load_snapshot_file();
		}
		return true;
	}

	log_index_t node::last_snapshot_index()
	{
		acl::lock_guard lg(metadata_locker_);
		return last_snapshot_index_;
	}
	void node::set_last_snapshot_index(log_index_t index)
	{
        logger("last_snapshot_index:%llu",index);
		acl::lock_guard lg(metadata_locker_);
		last_snapshot_index_ = index;
	}
	term_t node::last_snapshot_term()
	{
		acl::lock_guard lg(metadata_locker_);
		return last_snapshot_term_;
	}

	void node::set_last_snapshot_term(term_t term)
	{
		acl::lock_guard lg(metadata_locker_);
		last_snapshot_term_ = term;
	}

	void node::make_log_entry(const std::string &data, log_entry &entry)
	{
		term_t term = current_term();

		entry.set_term(term);
		entry.set_log_data(data);
		entry.set_type(e_raft_log);
	}

	bool node::write_log(const std::string &data,
		log_index_t &index, term_t &term)
	{

		log_entry entry;

		make_log_entry(data, entry);
		index = log_manager_->write(entry);
		term = entry.term();

		if (!index)
		{
			logger_error("log write error");
			return false;
		}

		if (should_compact_log())
			async_compact_log();

		return true;
	}

	void node::add_replicate_callback(const version& version,
		replicate_callback* callback)
	{
		acl::lock_guard lg(replicate_callbacks_locker_);
		replicate_callbacks_[version] = callback;
	}

	node::apply_log::apply_log(node& _node)
		:node_(_node),
		do_apply_(false),
		to_stop_(false)
	{
		acl_pthread_mutex_init(&mutex_, NULL);
		acl_pthread_cond_init(&cond_, NULL);
	}

	node::apply_log::~apply_log()
	{
		acl_pthread_mutex_lock(&mutex_);
		to_stop_ = true;
		acl_pthread_cond_signal(&cond_);
		acl_pthread_mutex_unlock(&mutex_);

		//wait thread;
		wait();
		acl_pthread_mutex_destroy(&mutex_);
		acl_pthread_cond_destroy(&cond_);
	}

	void node::apply_log::do_apply()
	{
		acl_pthread_mutex_lock(&mutex_);
		do_apply_ = true;
		acl_pthread_cond_signal(&cond_);
		acl_pthread_mutex_unlock(&mutex_);
	}

	bool node::apply_log::wait_to_apply()
	{

		acl_pthread_mutex_lock(&mutex_);
		if (!do_apply_ && !to_stop_)
			acl_pthread_cond_wait(&cond_, &mutex_);
		bool result = do_apply_;
		do_apply_ = false;
		acl_pthread_mutex_unlock(&mutex_);
		return result && !to_stop_;
	}

	void* node::apply_log::run()
	{
		while (wait_to_apply())
		{
			if (node_.is_leader())
				node_.invoke_replicate_callback(
					replicate_callback::E_OK);
			else
				node_.invoke_apply_callbacks();
		}
		return NULL;
	}

	node::log_compaction::log_compaction(node &_node)
		:node_(_node),
		do_compact_log_(false)
	{
		acl_pthread_mutex_init(&mutex_, NULL);
		acl_pthread_cond_init(&cond_, NULL);
	}

	node::log_compaction::~log_compaction()
	{
		acl_pthread_mutex_lock(&mutex_);
		if (do_compact_log_)
		{
			acl_pthread_cond_wait(&cond_, &mutex_);
		}
		acl_pthread_mutex_unlock(&mutex_);
	}

	void* node::log_compaction::run()
	{
		//set status true
		acl_pthread_mutex_lock(&mutex_);
		do_compact_log_ = true;
		acl_pthread_mutex_unlock(&mutex_);

		//do log compaction
		node_.do_compaction_log();

		//notify waiter
		acl_pthread_mutex_lock(&mutex_);
		do_compact_log_ = false;
		acl_pthread_cond_signal(&cond_);
		acl_pthread_mutex_unlock(&mutex_);

		return NULL;
	}

	void node::log_compaction::do_compact_log()
	{
		set_detachable(true);
		start();
	}

	node::election_timer::election_timer(node &_node)
		:node_(_node),
         stop_(false),
         cancel_(true),
         delay_(3600*1000)
	{
        acl_pthread_mutex_init(&mutex_, NULL);
        acl_pthread_cond_init(&cond_, NULL);
		start();
	}

	node::election_timer::~election_timer()
	{
		stop_ = true;
		cancel_timer();
		wait();
		acl_pthread_mutex_destroy(&mutex_);
        acl_pthread_cond_destroy(&cond_);
	}

	void node::election_timer::cancel_timer()
	{
        logger("cancel timer ");
		acl_pthread_mutex_lock(&mutex_);
		cancel_ = true;
		delay_ = 1000 * 60;
		acl_pthread_cond_broadcast(&cond_);
		acl_pthread_mutex_unlock(&mutex_);
	}

	void node::election_timer::set_timer(unsigned int delay)
	{
        logger("set timer %u",delay);
		acl_pthread_mutex_lock(&mutex_);
		delay_ = delay;
		cancel_ = false;
        acl_pthread_cond_broadcast(&cond_);
		acl_pthread_mutex_unlock(&mutex_);
	}

	void *node::election_timer::run()
	{
        logger("election timer to start run");

        timespec timeout;
        timeval now;

		while (!stop_)
		{
			gettimeofday(&now, NULL);
			timeout.tv_sec  =  now.tv_sec;
			timeout.tv_nsec =  now.tv_usec * 1000;
			timeout.tv_sec  += delay_ / 1000;
			timeout.tv_nsec += (delay_ % 1000) * 1000 * 1000;

			acl_assert(!acl_pthread_mutex_lock(&mutex_));


			int status = acl_pthread_cond_timedwait(
				&cond_,
				&mutex_,
				&timeout);

			if (cancel_ || status != ACL_ETIMEDOUT)
            {
                acl_assert(!acl_pthread_mutex_unlock(&mutex_));
                continue;
			}
			acl_assert(!acl_pthread_mutex_unlock(&mutex_));

			node_.election_timer_callback();
		}

		return NULL;
	}
}
