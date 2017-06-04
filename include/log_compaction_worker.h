#pragma once
namespace raft
{
	class node;
	class log_compaction_worker: private acl::thread
	{
	public:
		log_compaction_worker(node &_node);
		void compaction_log();
	private:
		virtual void* run();
		node &node_;
	};
}