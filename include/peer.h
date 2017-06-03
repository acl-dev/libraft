#pragma once
namespace raft
{
	class node;

	class peer :public acl::thread
	{
	public:
		enum status_t
		{
			E_IDLE,//idle
			E_REPLICATING,//replicating
			E_VOTING, //voting,
			E_SLEEPING,//sleep
		};
		peer(node &_node);
		bool notify_repliate();
		bool notify_vote();
	private:
		bool check_heartbeart();

		bool check_do_replicate();
		void do_replicate();

		bool check_do_vote();
		void do_vote();

		virtual void* run();


		node		&node_;
		acl::locker locker_;
		status_t	status_;
		log_index_t match_index_;
		log_index_t next_index_;

		bool to_replicate_;
		bool to_vote_;


		acl_pthread_cond_t *cond_;
		acl_pthread_mutex_t *mutex_;

		timeval last_replicate_time_;

		size_t heart_inter_;
	};
}