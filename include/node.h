#pragma once
namespace raft
{
	enum status_t
	{
		E_OK,
		E_NO_LEADER,
		E_TIMEOUT,
		E_UNKNOWN,
		E_WRITE_LOG_ERROR,
	};

	struct version
	{
		log_index_t index_;
		term_t term_;
	};

	struct replicate_cond_t;
	class node
	{
	public:
		
		node();
		
		std::pair<status_t, version> replicate(
			const std::string &data, unsigned int timeout_millis);
		
		bool is_leader();

		void bind_snapshot_callback(snapshot_callback *callback);

		void set_snapshot_path(const std::string &path);

		void set_log_path(const std::string &path);

		void set_metadata_path(const std::string &path);

		void set_max_log_size(size_t size);

		void set_max_log_count(size_t size);

		std::string raft_id();

	private:
		enum role_t
		{
			E_LEADER,//leader
			E_FOLLOWER,//follower
			E_CANDIDATE//candidate
		};

		friend class peer;
		friend class log_compaction_worker;
		friend class election_timer;
		
		void init();

		//for peer
		bool is_candicate();

		log_index_t get_last_log_index();

		log_index_t gen_log_index();

		term_t current_term();
		
		void update_term(term_t term);

		void update_leader_id(const std::string &leader_id);

		role_t role();

		void update_role(role_t _role);

		log_index_t last_log_index();

		bool build_replicate_log_request(
			replicate_log_entries_request &requst, 
			log_index_t index,
			int entry_size = 0);

		std::vector<log_index_t> get_peers_match_index();

		void replicate_log_callback();

		void build_vote_request(vote_request &req);

		void clear_vote_response();

		void vote_response_callback(const std::string &peer_id, 
			const vote_response &response);

		int peers_count();

		void handle_new_term_callback(term_t term);

		bool get_snapshot(std::string &path);
		
		// log compaction things
		std::map<log_index_t, std::string> 
			scan_snapshots();

		bool should_compact_log();
		
		bool check_compacting_log();

		void async_compact_log();

		bool make_snapshot();

		void do_compaction_log();
		//

		void become_leader();

		void update_committed_index(log_index_t index);

		void set_election_timer();

		void cancel_election_timer();

		void election_timer_callback();

		log_index_t committed_index();

		log_index_t start_log_index();

		//about peers function
		void notify_peers_replicate_log();

		void notify_peers_to_election();

		void update_peers_next_index();
		//end
		void step_down();

		void load_snapshot_file();

		acl::fstream *get_snapshot_tmp(
			const snapshot_info &);

		void close_snapshot();

		bool handle_vote_request(
			const vote_request &req, 
			vote_response &resp);

		bool handle_replicate_log_request(
			const replicate_log_entries_request &req, 
			replicate_log_entries_response &resp);

		bool handle_install_snapshot_requst(
			const install_snapshot_request &req, 
			install_snapshot_response &resp);

		void add_waiter(replicate_cond_t *waiter);

		void remove_waiter(replicate_cond_t *waiter);

		void make_log_entry(const std::string &data,
			log_entry &entry);

		bool write_log(const std::string &data, 
			log_index_t &index);

		void notify_replicate_conds(log_index_t index,
			status_t = status_t::E_OK);


		log_manager *log_manager_;

		unsigned int election_timeout_;
		log_index_t last_log_index_;
		log_index_t committed_index_;
		term_t		current_term_;
		role_t		role_;
		std::string raft_id_;
		std::string leader_id_;
		std::string vote_for_;
		acl::locker	metadata_locker_;

		typedef std::map<log_index_t, 
			replicate_cond_t*> replicate_conds_t;

		replicate_conds_t replicate_conds_;
		acl::locker waiters_locker_;


		std::map<std::string, peer*> peers_;
		acl::locker peers_locker_;


		snapshot_callback	*snapshot_callback_;
		std::string			snapshot_path_;
		snapshot_info		*snapshot_info_;
		acl::fstream		*snapshot_tmp_;
		acl::locker			snapshot_locker_;

		std::string log_path_;
		std::string metadata_path_;

		size_t max_log_size_;
		size_t max_log_count_;

		bool		compacting_log_;
		acl::locker compacting_log_locker_;

		typedef std::map<std::string, 
			vote_response> vote_responses_t;

		vote_responses_t vote_responses_;
		acl::locker		 vote_responses_locker_;

		election_timer election_timer_;

		log_compaction_worker log_compaction_worker_;

		
	};
}