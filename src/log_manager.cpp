#include <cstring>
#include <map>
#include "raft.hpp"

#ifndef __LOG_EXT__ 
#define __LOG_EXT__ ".log"
#endif // !__LOG_EXT__ 

namespace raft
{

	log_manager::log_manager(const std::string &path) 
		:path_(path)
	{
		if (path_.back() != '/' && path_.back() != '\\')
			path_.push_back('/');

		log_size_	= 4 * 1024 * 1024;
		last_index_ = 0;
		last_log_	= NULL;
		last_term_	= 0;
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

		if (!last_log_ || (index = last_log_->write(entry)) == 0)
		{
			if (last_log_)
				acl_assert(last_log_->eof());

			acl::string filepath(path_.c_str());

			filepath.format_append("%llu%s",last_index_ + 1, __LOG_EXT__);

			last_log_ = create(filepath.c_str());
			if (!last_log_)
			{
				logger_error("create log error");
				return 0;
			}

			if ((index = last_log_->write(entry)) == 0)
			{
				logger_error("write log error");
				last_log_->dec_ref();
				last_log_ = NULL;
				return 0;
			}
			log_index_t start_index = last_log_->start_index();
			logs_.insert(std::make_pair(start_index, last_log_));
		}
		//update begin ,term. 
		last_index_ = index;
		last_term_ = entry.term();

		return index;
	}

	bool log_manager::read(log_index_t index, log_entry &entry)
	{
		bool result = false;

		if (index > last_index() || index < start_index() || !log_count())
			return false;
		
		log *log_ = find_log(index);
		acl_assert(log_);

		result = log_->read(index, entry);
		log_->dec_ref();

		return result;
	}

	void log_manager::truncate(log_index_t index)
	{
		acl::lock_guard lg(locker_);
		std::map<log_index_t, log*>::iterator it = logs_.begin();
		for(;it != logs_.end();)
		{
			log* _log = it->second;
			if(_log->last_index() <= index)
			{
				_log->auto_delete(true);
				_log->dec_ref();
				it = logs_.erase(it);
				continue;
			}
			_log->truncate(index);
			return;
		}
	}

	bool log_manager::read(log_index_t index, 
		int max_bytes, 
		int max_count, 
		std::vector<log_entry> &entries)
	{
		int bytes = 0;
		log_index_t begin = index;

		do
		{
			if ( max_bytes <= 0 || max_count <= 0 || begin > last_index())
				break;

			log *log_ = find_log(begin);
			
			if (!log_)
				break;

			if (!log_->read(
				begin,
				max_bytes,
				max_count,
				entries,
				bytes))
			{
				logger_error("read log error");
				log_->dec_ref();
				break;
			}
			log_->dec_ref();
			begin = index + static_cast<int>(entries.size());
			max_count -= static_cast<int>(entries.size());
			max_bytes -= bytes;
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
		/*
		 * when log_manager empty. and last_index_ eq start_index_;
		 */
		return last_index_;
	}

	raft::log_index_t log_manager::last_index()
	{
		acl::lock_guard lg(locker_);
		return last_index_;
	}

	term_t log_manager::last_term()
	{
		acl::lock_guard lg(locker_);
		return last_term_;
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

	int log_manager::discard_log(log_index_t last_index)
	{
		typedef std::map<log_index_t, log*>::iterator iterator_t;

		acl::lock_guard lg(locker_);
		int del_count_ = 0;
		iterator_t it = logs_.begin();

		for(; it!= logs_.end(); ++it)
		{
			if (it->second->last_index() <= last_index)
			{
				std::string filepath = it->second->file_path();
				it->second->auto_delete(true);
				it->second->dec_ref();
				it = logs_.erase(it);
				del_count_++;
				logger("log_manager discard %s log", filepath.c_str());
			}
			else
				break;
		}

		return del_count_;
	}

	void log_manager::set_log_size(int log_size)
	{
		log_size_ = log_size;
	}

	void log_manager::set_last_index(log_index_t index)
	{
		acl::lock_guard lg(locker_);
		last_index_ = index;
	}

	void log_manager::set_last_term(term_t term)
	{
		acl::lock_guard lg(locker_);
		last_term_ = term;
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
		const char* file_path = NULL;

		std::map<log_index_t, log*>::iterator it = logs_.begin();

		for (; it != logs_.end(); ++it)
		{
			it->second->inc_ref();
		}
		logs_.clear();

		if (scan.open(path_.c_str(), false) == false)
		{
			logger_error("scan open error %s\r\n",
				acl::last_serror());
			return;
		}

		while ((file_path = scan.next_file(true)) != NULL)
		{
			if (acl_strrncasecmp(file_path,
				__LOG_EXT__, strlen(__LOG_EXT__)) == 0)
			{
				std::string logfile= std::string(file_path);
				log *_log = create(logfile);
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
		if(logs_.size())
		{
			last_index_ = logs_.rbegin()->second->last_index();
		}
	}

}