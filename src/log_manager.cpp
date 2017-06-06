#include "raft.hpp"

#ifndef __LOG_EXT__ 
#define __LOG_EXT__ ".log"
#endif // !__LOG_EXT__ 

namespace raft
{

	log_manager::log_manager(const std::string &path) 
		:path_(path)
	{
		if (path_.back() != '/' ||
			path_.back() != '\\')
			path_.push_back('/');

		log_size_ = 4 * 1024 * 1024;
		reload_logs();
	}

	log_manager::~log_manager()
	{
		acl::lock_guard lg(locker_);

		std::map<log_index_t, log*>::iterator it = logs_.begin();
		for (; it != logs_.end(); ++it)
		{
			it->second->dec_ref();
		}
	}
	
	log_index_t log_manager::write(const log_entry &entry)
	{
		log_index_t index = 0;

		acl::lock_guard lg(locker_);

		if (last_log_ || (index = last_log_->write(entry)) == 0)
		{
			acl::string filepath(path_.c_str());

			filepath.format_append("%llu%s",
				last_index_no_lock() + 1, __LOG_EXT__);

			last_log_ = create(filepath.c_str());
			if (!last_log_)
			{
				logger_error("create log error");
				return false;
			}

			if ((index = last_log_->write(entry)) == 0)
			{
				logger_error("write log error");
				last_log_->dec_ref();
				last_log_ = NULL;
				return false;
			}
			log_index_t index = last_log_->start_index();
			logs_.insert(std::make_pair(index, last_log_));
			return true;
		}
		return index;
	}

	bool log_manager::read(log_index_t index, log_entry &entry)
	{
		bool result = false;

		if (index > last_index())
			return false;

		log *log_ = find_log(index);
		result = log_->read(index, entry);
		log_->dec_ref();

		return result;
	}

	bool log_manager::read(log_index_t index, 
		int max_bytes, 
		int max_count, 
		std::vector<log_entry> &entries)
	{
		int bytes = 0;

		do
		{
			if ( max_bytes - bytes <= 0 || max_count - entries.size() <= 0)
				break;

			if(index > last_index())
				break;

			log *log_ = find_log(index);
			
			if (!log_)
				break;

			if (!log_->read(
				index,
				max_bytes - bytes,
				max_count - (int)entries.size(),
				entries,
				bytes))
			{
				logger_error("read log error");
				log_->dec_ref();
				break;
			}
			log_->dec_ref();
		} while (true);

		return !entries.empty();
	}

	size_t log_manager::log_count()
	{
		acl::lock_guard lg(locker_);
		return logs_.size();
	}

	raft::log_index_t log_manager::start_index()
	{
		acl::lock_guard lg(locker_);

		if (logs_.size())
			return logs_.begin()->first;
		return 0;
	}

	raft::log_index_t log_manager::last_index()
	{
		acl::lock_guard lg(locker_);
		return last_index_no_lock();
	}

	std::map<log_index_t, log_index_t> log_manager::logs_info()
	{
		acl::lock_guard lg(locker_);

		std::map<log_index_t, log_index_t> infos;
		std::map<log_index_t, log*>::iterator it = logs_.begin();
		for (; it != logs_.end(); ++it)
		{
			infos[it->first] = it->second->last_index();
		}
		return infos;
	}

	bool log_manager::destroy_log(log_index_t log_start_index)
	{
		typedef std::map<log_index_t, log*>::iterator iterator_t;

		acl::lock_guard lg(locker_);

		iterator_t it = logs_.find(log_start_index);

		if (it == logs_.end())
			return false;

		it->second->auto_delete(true);
		logs_.erase(it);
		return true;
	}

	void log_manager::set_log_size(int log_size)
	{
		log_size_ = log_size;
	}

	raft::log_index_t log_manager::last_index_no_lock()
	{
		if (logs_.empty())
			return 0;
		return logs_.rbegin()->second->last_index();
	}

	log * log_manager::find_log(log_index_t index)
	{
		acl::lock_guard lg(locker_);

		if (last_log_ && index >= last_log_->start_index())
		{
			last_log_->inc_ref();
			return last_log_;
		}

		std::map<log_index_t, log*>::reverse_iterator it = logs_.rbegin();
		for (; it != logs_.rend(); ++it)
		{
			if (it->first <= index)
			{
				it->second->inc_ref();
				return it->second;
			}
		}
		return NULL;
	}

	void log_manager::reload_logs()
	{
		acl::lock_guard lg(locker_);

		acl::scan_dir scan;
		const char* filepath = NULL;

		std::map<log_index_t, log*>::iterator it = logs_.begin();

		for (; it != logs_.end(); ++it)
		{
			delete it->second;
		}
		logs_.clear();

		if (scan.open(path_.c_str(), false) == false)
		{
			logger_error("scan open error %s\r\n",
				acl::last_serror());
			return;
		}

		while ((filepath = scan.next_file(true)) != NULL)
		{
			if (acl_strrncasecmp(filepath, 
				__LOG_EXT__, strlen(__LOG_EXT__)) == 0)
			{
				log *_log = create(filepath);
				//delete empty log
				if (_log->empty())
				{
					_log->auto_delete(true);
					_log->dec_ref();
					continue;
				}
				acl_assert(logs_.insert(
					std::make_pair(_log->start_index(), _log)).second);
			}
		}
	}

}