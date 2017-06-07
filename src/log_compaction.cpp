#include "raft.hpp"
namespace raft
{

	log_compaction::log_compaction(node &_node)
		:node_(_node)
	{

	}

	void* log_compaction::run()
	{
		node_.do_compaction_log();
		return NULL;
	}

	void log_compaction::do_compact_log()
	{
		set_detachable(true);
		start();
	}

}