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

	struct replicate_waiter_t
	{
		acl_pthread_cond_t *cond_;
		acl_pthread_mutex_t *mutex_;
		status_t result_;
		log_index_t log_index_;

		replicate_waiter_t();
		~replicate_waiter_t();
	};

	class node
	{
	public:
		
		node();
		
		std::pair<status_t, version>
			replicate(const std::string &data, int timeout_millis);
		
		bool is_leader();

	private:
		enum role
		{
			E_LEADER,//leader
			E_FOLLOWER,//follower
			E_CANDIDATE//Candidate
		};

		friend class peer;


		//for peer
		bool is_candicate();

		log_index_t get_last_log_index();

		term_t get_current_term();
		
		log_index_t last_log_index();

		bool build_replicate_log_request(
			replicate_log_entries_request &requst, 
			log_index_t index,
			int entry_size = 0);

		void replicate_log_callback();

		bool build_vote_request(vote_request &req);

		void vote_response_callback(const vote_response &response);

		void handle_new_term_callback(term_t term);

		bool get_snapshot(std::string &path);
		//

		void notify_peers_replicate_log();

		bool handle_vote_request(const vote_request &req, vote_response &resp);

		bool handle_replicate_log_request(
			const replicate_log_entries_request &req, 
			replicate_log_entries_response &resp);

		bool handle_install_snapshot_requst(
			const install_snapshot_request &req, 
			install_snapshot_response &resp);

		void add_waiter(replicate_waiter_t *waiter);

		void make_log_entry(const std::string &data, log_entry &entry);
		bool write_log(const std::string &data, log_index_t &index);

		void signal_waiter();

		void init();
		bool do_commit();

		log_manager<mmap_log_creater> log_;

		log_index_t last_log_index_;
		log_index_t committed_index_;
		term_t			  current_term_;
		acl::locker		  metadata_locker_;


		std::map<log_index_t, replicate_waiter_t*>  replicate_waiters_;
		acl::locker waiters_locker_;

		std::string raft_id_;

		std::map<std::string, peer*> peers_;
		acl::locker peers_locker_;

		role role_;
	};
}