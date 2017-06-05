#pragma once
namespace raft
{
	class node;
	class vote_timer: private acl::thread
	{
	public:
		vote_timer(node &_node);
		~vote_timer();
		void set_timer(size_t milliseconds);
		void stop_timer();
	private:
		node &node_;
	};
}