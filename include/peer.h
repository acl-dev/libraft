#pragma once
namespace raft
{
	class node;

	class peer :public acl::thread
	{
	public:
		peer(node &_node, const std::string &peer_id);

		~peer();

		void notify_repliate();
		
		void notify_election();

		log_index_t match_index();

		void set_next_index(log_index_t index);

		void set_match_index(log_index_t index);

	private:
		void notify_stop();

		void do_replicate();

		bool do_install_snapshot();

		void do_election()const;

		bool wait_event(int &event);

		virtual void* run();

	private:
		node		&node_;
		std::string peer_id_;
		acl::locker locker_;

		log_index_t match_index_;
		log_index_t next_index_;

		int event_;

		acl_pthread_cond_t *cond_;
		acl_pthread_mutex_t *mutex_;
		
		timeval last_replicate_time_;
		long long heart_inter_;
		
		acl::string replicate_service_path_;
		acl::string election_service_path_;
		acl::string install_snapshot_service_path_;

		acl::http_rpc_client *rpc_client_;
		size_t rpc_faileds_;
		size_t req_id_;
		
	};
}