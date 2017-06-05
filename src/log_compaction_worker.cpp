#include "raft.hpp"
namespace raft
{

	log_compaction_worker::log_compaction_worker(node &_node)
		:node_(_node)
	{

	}

	void* log_compaction_worker::run()
	{
		node_.do_compaction_log();
		return NULL;
	}

	void log_compaction_worker::compact_log()
	{
		set_detachable(true);
		start();
	}

}