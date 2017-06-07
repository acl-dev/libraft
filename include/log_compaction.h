#pragma once
namespace raft
{
	class node;
	class log_compaction: private acl::thread
	{
	public:
		log_compaction(node &_node);
		void do_compact_log();
	private:
		virtual void* run();
		node &node_;
	};
}