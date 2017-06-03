#pragma once
namespace raft
{
	template <class CreateLogPolicy>
	class log_manager
	{
	public:
		log_manager()
		{

		}
		bool write(const log_entry &entry)
		{
			acl::lock_guard lg(locker_);

			if (!current_wlog_)
			{
				return create_log(entry);
			}
			else if (!current_wlog_->write(entry))
			{
				if (!current_wlog_->eof())
				{
					logger_error("write log error");
					return false;
				}
				return create_log(entry);
			}
			return true;
		}
		bool read(log_index_t index, 
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
		bool read(log_index_t index, log_entry &entry)
		{
			log *log_ = find_log(index);
			if (!log_)
			{
				logger("not find log ,index: %d",index);
				return false;
			}
			return log_->read(index, entry);
		}
	private:
		log *find_log(log_index_t index)
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
		bool create_log(const log_entry &entry)
		{
			//create new log 
			std::string filepath(path_);

			filepath += std::to_string(entry.index()) +  ".log";
			current_wlog_ = CreateLogPolicy::create(filepath);
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
		void reload_log()
		{
			acl::lock_guard lg(locker_);
			logs_ = CreateLogPolicy::reload(path_);
			if (logs_.empty())
				return;
			//the newest one 
			current_wlog_ = logs_.rbegin()->second;
		}


		std::string path_;

		acl::locker locker_;
		log *current_wlog_;
		std::map<log_index_t, log*> logs_;
	};
}