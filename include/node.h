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
	struct replicate_cond_t;

	struct version
	{
		log_index_t index_;
		term_t term_;
	};

	struct replicate_callback
	{
		virtual ~replicate_callback() {};

		virtual bool commit(const std::string& data, const version& ver) = 0;
	};
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

		std::string raft_id()const;

	private:
		enum role_t
		{
			E_LEADER,//leader
			E_FOLLOWER,//follower
			E_CANDIDATE//candidate
		};

		friend class peer;
		friend class log_compaction;
		friend class election_timer;
		
		void init();

		//for peer
		bool is_candicate();

		log_index_t gen_log_index();

		log_index_t last_log_index();

		void set_last_log_index(log_index_t index);

		log_index_t last_snapshot_index();

		void set_last_snapshot_index(log_index_t index);

		term_t last_snapshot_term();

		void set_last_snapshot_term(term_t term);

		term_t current_term();
		
		void set_current_term(term_t term);

		std::string leader_id();

		void set_leader_id(const std::string &leader_id);

		log_index_t committed_index();

		void set_committed_index(log_index_t index);

		role_t role();

		void set_role(role_t _role);

		std::string vote_for();

		void set_vote_for(const std::string &vote_for);

		void set_apply_index(log_index_t index);

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

		void handle_new_term(term_t term);

		bool get_snapshot(std::string &path) const;
		
		// log compaction things
		std::map<log_index_t, std::string> 
			scan_snapshots() const;

		bool should_compact_log();
		
		bool check_compacting_log();

		void async_compact_log();

		bool make_snapshot() const;

		void do_compaction_log();
		//

		void become_leader();

		void set_election_timer();

		void cancel_election_timer();

		void election_timer_callback();


		log_index_t start_log_index()const;

		//about peers function
		void notify_peers_replicate_log();

		void notify_peers_to_election();

		void update_peers_next_index(log_index_t index);
		//end
		void step_down();

		void load_snapshot_file();

		acl::fstream *get_snapshot_tmp(const snapshot_info &);

		void close_snapshot();

		void do_apply_log();

		bool handle_vote_request(const vote_request &req, vote_response &resp);

		bool handle_replicate_log_request(
			const replicate_log_entries_request &req, 
			replicate_log_entries_response &resp);

		bool handle_install_snapshot_requst(
			const install_snapshot_request &req, 
			install_snapshot_response &resp);

		void add_waiter(replicate_cond_t *waiter);

		void remove_waiter(replicate_cond_t *waiter);

		void make_log_entry(const std::string &data, log_entry &entry);

		bool write_log(const std::string &data, 
			log_index_t &index, term_t &term);

		void notify_replicate_conds(log_index_t index, 
			status_t = status_t::E_OK);

		void update_peers_match_index(log_index_t index);
	private:
		class apply_log : private acl::thread
		{
		public:
			apply_log(node &);
			~apply_log();
			void do_apply();
			virtual void *run();
		private:
			bool wait_to_apply();
			node &node_;
			bool do_apply_;
			bool to_stop_;
			acl_pthread_mutex_t *mutex_;
			acl_pthread_cond_t *cond_;
		};
	private:
		log_manager *log_manager_;

		unsigned int election_timeout_;
		log_index_t last_log_index_;
		log_index_t committed_index_;
		log_index_t applied_index_;
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
		log_index_t			last_snapshot_index_;
		term_t				last_snapshot_term_;

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

		log_compaction log_compaction_worker_;

		replicate_callback *replicate_callback_;

		apply_log apply_log_;
	};
}
