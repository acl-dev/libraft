#include "raft.hpp"

#ifndef __INDEX__EXT__
#define __INDEX__EXT__ ".index"
#endif // __INDEX__EXT__

namespace raft
{
	mmap_log_manager::mmap_log_manager(const std::string &log_path)
		:log_manager(log_path)
	{

	}

	log *mmap_log_manager::create(const std::string &filepath)
	{

		log *_log = new mmap_log(last_index_no_lock(), log_size_);

		if (!_log->open(filepath))
		{
			logger_error("mmap_log open error,%s",
				filepath.c_str());
			return NULL;
		}
		return _log;
	}
}