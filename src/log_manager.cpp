#include "raft.hpp"
namespace raft
{

	log_manager::log_manager(const std::string &path) 
		:path_(path)
	{
		reload_logs();
	}

	bool log_manager::write(const log_entry &entry)
	{
		acl::lock_guard lg(locker_);

		if (!current_wlog_)
		{
			return write_new_log(entry);
		}
		else if (!current_wlog_->write(entry))
		{
			if (!current_wlog_->eof())
			{
				logger_error("write log error");
				return false;
			}
			return write_new_log(entry);
		}
		return true;
	}

	bool log_manager::read(log_index_t index, log_entry &entry)
	{
		log *log_ = find_log(index);
		if (!log_)
		{
			logger("not find log ,index: %d", index);
			return false;
		}
		return log_->read(index, entry);
	}

	bool log_manager::read(log_index_t index, 
		int max_bytes, 
		int max_count, 
		std::vector<log_entry> &entries)
	{
		int bytes = 0;

		do
		{
			if ((max_bytes - bytes) <= 0 ||
				(max_count - entries.size()) <= 0)
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
				break;
			}

		} while (true);

		return !entries.empty();
	}

	size_t log_manager::log_count()
	{
		return logs_.size();
	}

	raft::log_index_t log_manager::start_log_index()
	{
		acl::lock_guard lg(locker_);
		if (logs_.empty())
			return 0;

		logs_.begin()->first;
	}

	std::map<log_index_t, log_index_t> log_manager::logs_info()
	{
		acl::lock_guard lg(locker_);

		std::map<log_index_t, log_index_t> infos;
		std::map<log_index_t, log*>::iterator it = logs_.begin();
		for (; it != logs_.end(); ++it)
		{
			if (it->first > 0 && it->second->last_index() > 0)
			{
				infos[it->first] = it->second->last_index();
			}
			else
			{
				logger_fatal("log_manager error");
			}
		}
		return infos;
	}

	bool log_manager::destroy_log(log_index_t log_start_index)
	{
		acl::lock_guard lg(locker_);

		std::map<log_index_t, log*>::iterator it =
			logs_.find(log_start_index);

		if (it == logs_.end())
			return false;

		acl_assert(destroy_log(it->second));
		logs_.erase(it);
		return true;
	}

	log * log_manager::find_log(log_index_t index)
	{
		acl::lock_guard lg(locker_);

		if (current_wlog_ && index >=
			current_wlog_->start_index())

			return current_wlog_;

		std::map<log_index_t, log*>::reverse_iterator
			it = logs_.rbegin();

		for (; it != logs_.rend(); ++it)
		{
			if (it->first <= index)
			{
				return it->second;
			}
		}
		return NULL;
	}

	bool log_manager::write_new_log(const log_entry &entry)
	{
		//create new log 
		std::string filepath(path_);

		filepath += std::to_string(entry.index()) + __LOG_EXT__;
		current_wlog_ = create(filepath);
		if (!current_wlog_)
		{
			logger_error("create log error");
			return false;
		}

		if (!current_wlog_->write(entry))
		{
			logger_error("write log error");
			current_wlog_->close();
			delete current_wlog_;
			current_wlog_ = NULL;
			return false;
		}
		log_index_t index = current_wlog_->start_index();
		logs_.insert(std::make_pair(index, current_wlog_));
		return true;
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
				__LOG_EXT__,
				strlen(__LOG_EXT__)) == 0)
			{
				log *_log = create(filepath);
				//delete empty log
				if (_log->start_index() == 0 ||
					_log->last_index() == 0)
				{
					destroy_log(_log);
					continue;
				}
				acl_assert(logs_.insert(
					std::make_pair(_log->start_index(), _log)).second);
			}
		}
	}

}