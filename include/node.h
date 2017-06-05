#pragma once
namespace raft
{
	enum status_t
	{
		E_OK,
		E_NO_LEADER,
		E_TIMEOUT,
		E_UNKNOWN,
	};

	struct version 
	{
		log_index_t index_;
		term_t term_;
	};

	struct replicate_waiter_t;
	class node
	{
	public:
		
		node();
		
		std::pair<status_t, version> replicate(
			const std::string &data,int timeout_millis);
		
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

		//for peer
		bool is_candicate();

		log_index_t get_last_log_index();

		term_t current_term();
		
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

		void vote_response_callback(const std::string &peer_id, 
			const vote_response &response);

		int peers_count();

		void handle_new_term_callback(term_t term);

		bool get_snapshot(std::string &path);
		
		// log compaction things
		std::map<log_index_t, std::string> scan_snapshots();

		bool check_log_compaction();
		
		bool check_making_snapshot();

		void async_log_compaction();

		void set_making_snapshot();

		void do_compaction_log();
		//

		void become_leader();

		void update_committed_index(log_index_t index);

		void election_timer_callback();

		log_index_t committed_index();

		log_index_t start_log_index();

		void notify_peers_replicate_log();

		void update_peers_next_index();

		bool handle_vote_request(
			const vote_request &req, 
			vote_response &resp);

		bool handle_replicate_log_request(
			const replicate_log_entries_request &req, 
			replicate_log_entries_response &resp);

		bool handle_install_snapshot_requst(
			const install_snapshot_request &req, 
			install_snapshot_response &resp);

		void add_waiter(replicate_waiter_t *waiter);

		void remove_waiter(replicate_waiter_t *waiter);

		void make_log_entry(const std::string &data, log_entry &entry);

		bool write_log(const std::string &data, log_index_t &index);

		void signal_replicate_waiter(log_index_t index);

		void init();

		bool do_commit();

		log_manager *log_manager_;

		log_index_t last_log_index_;
		log_index_t committed_index_;
		term_t		current_term_;
		role_t		role_;
		acl::locker	metadata_locker_;


		std::map<log_index_t, replicate_waiter_t*>  replicate_waiters_;
		acl::locker waiters_locker_;

		std::string raft_id_;

		std::map<std::string, peer*> peers_;

		acl::locker peers_locker_;


		snapshot_callback *snapshot_callback_;

		std::string snapshot_path_;

		std::string log_path_;

		std::string metadata_path_;

		size_t max_log_size_;

		size_t max_log_count_;

		bool making_snapshot_;

		acl::locker making_snapshot_locker_;

		//
		std::map<std::string, vote_response> vote_responses_;
		
		acl::locker vote_responses_locker_;


	};
}