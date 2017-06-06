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
	mmap_log_manager::~mmap_log_manager()
	{

	}
	log *mmap_log_manager::create(const std::string &filepath)
	{

		log *_log = new mmap_log();

		if (!_log->open(filepath))
		{
			logger_error("mmap_log open error,%s",
				filepath.c_str());
			return NULL;
		}
		return _log;
	}

	void mmap_log_manager::release_log(log *&_log)
	{
		_log->close();
		delete _log;
		_log = NULL;
	}

	bool mmap_log_manager::destroy_log(log *_log)
	{
		std::string file_path = _log->file_path();
		_log->close();
	}
}