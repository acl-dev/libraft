#pragma once
namespace raft
{
	class node;

	class peer :public acl::thread
	{
	public:
		peer(node &_node);

		void notify_repliate();
		
		void notify_vote();

	private:
		bool check_heartbeart();

		bool check_do_replicate();
		void do_replicate();

		bool do_install_snapshot();

		bool check_do_vote();
		void do_vote();

		void to_sleep();

		virtual void* run();

		node		&node_;
		acl::locker locker_;

		log_index_t match_index_;
		log_index_t next_index_;

		bool to_replicate_;
		bool to_vote_;


		acl_pthread_cond_t *cond_;
		acl_pthread_mutex_t *mutex_;

		timeval last_replicate_time_;

		long long heart_inter_;

		std::string replicate_service_path_;

		acl::http_rpc_client *rpc_client_;

		size_t rpc_faileds_;
	};
}