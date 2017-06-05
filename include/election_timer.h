#pragma once
namespace raft
{
	class node;
	class election_timer: acl::thread
	{
	public:
		election_timer(node &_node);
		
		~election_timer();

		void set_timer(unsigned int delays_mills);

		void cancel_timer();
	private:
		virtual void* run();
		node &node_;
		bool cancel_;
		bool stop_;
		unsigned int delay_;
		acl_pthread_mutex_t *mutex_;
		acl_pthread_cond_t *cond_;
	};
}