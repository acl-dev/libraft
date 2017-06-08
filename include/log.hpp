#pragma once
namespace raft
{
	struct log 
	{
		/**
		 * \brief log construct ,it must be new.
		 * because dec_ref() will invoke log self to delete itself
		 * and set ref to 1.not zero
		 * and delete log.invoke dec_ref()
		 */
		log()
		{
			auto_delete_ = false;
			ref_ = 1;
		}
		/**
		 * \brief open log ,if file exist just open it,if not exist
		 * it will create new log file
		 * \param filepath filepath to store log. 
		 * \return if open ok return true,otherwise return false
		 */
		virtual bool open(const std::string &filepath) = 0;

		/**
		 * \brief get log file path
		 * \return if open ok,return filepath,otherwise return empty
		 */
		virtual std::string file_path() = 0;
		
		/**
		 * \brief write a log_entry to log file
		 * \param entry log_entry
		 * \return return log index ( > 0) if write ok.otherwise return 0;
		 */
		virtual log_index_t write(const log_entry &entry) = 0;
		
		/**
		 * \brief truncate log, and it will delete log entry [index, last_index]
		 * \param index index to delete
		 * \return return true if truncate ok,otherwise return false
		 */
		virtual bool truncate(log_index_t index) = 0;
		

		/**
		 * \brief read one log entry
		 * \param index the index of log entry to be readed
		 * \param entry entry to store log data
		 * \return return true if read log entry ok
		 * return false, something error happend
		 */
		virtual bool read(log_index_t index, log_entry &entry) = 0;

		/**
		 * \brief read log entries from log
		 * \param index log index to read
		 * \param max_bytes max bytes copy to entries buffer
		 * \param max_count max count of entries to read
		 * \param entries buffer to store log entries
		 * \param bytes size of all the log entries data
		 * \return true if read ok, otherwise return false
		 */
		virtual bool read(log_index_t index, 
						  int max_bytes, 
						  int max_count, 
						  std::vector<log_entry> &entries, 
						  int &bytes) = 0;

		/**
		 * \brief get last index of this log
		 * \return return 0,if empty otherwise return log_index_t ( > 0)
		 */
		virtual log_index_t last_index() = 0;

		/**
		 * \brief get start index of this log file
		 * \return return > 0 if 
		 */
		virtual log_index_t start_index() = 0;
		
		/**
		 * \brief check if end of file
		 * \return return true if log is end of file
		 * otherwise return false
		 */
		virtual bool eof() = 0;

		/**
		 * \brief check if log is empty
		 * \return return true if empty ,
		 * otherwise return false for not empty
		 */
		virtual bool empty() = 0;

		/**
		 * \brief set log file auto delete file from disk
		 * when log obj is delete and it will delete file 
		 * from disk
		 * \param val eq true ,set log auto delete file from
		 * disk,otherwise val is false.default log will not 
		 * delete file from disk when delete log obj
		 */
		virtual void auto_delete(bool val)
		{
			acl::lock_guard lg(locker_);
			auto_delete_ = val;
		}

		/**
		 * \brief check log auto delete file
		 * \return return true if log auto delete file,
		 * otherwise return false
		 */
		virtual bool auto_delete()
		{
			acl::lock_guard lg(locker_);
			return auto_delete_;
		}

		/**
		 * \brief log use ref to manager log live time
		 * if inc_ref() invoke ref ++, dec_inf() ref --
		 * if ref == 0,it will release itself.
		 */
		void inc_ref()
		{
			acl::lock_guard lg(locker_);
			ref_++;
		}

		/**
		 * \brief log use ref to manager live time.
		 * dec_ref() invoke ,ref = ref -1, if ref ==0
		 * it will invoke close() ,and delete itself
		 */
		void dec_ref()
		{
			locker_.lock();
			ref_--;
			int ref = ref_;
			locker_.unlock();

			if (ref == 0)
				close();
		}

		/**
		 * \brief get current log ref
		 * \return return current ref.
		 */
		int ref()
		{
			acl::lock_guard lg(locker_);
			return ref_;
		}
	protected:
		/**
		 * \brief private destruct,meam that only log self can
		 * delete itself.others can't delete log,but can dec_ref()
		 * to manager it live time
		 */
		virtual ~log() {};
		/**
		 * \brief when log to to closed,it will invote to close(),
		 * and in the function, it will relase resources,eg:delete file,
		 * release memory...
		 */
		virtual void close() = 0;

		/**
		 * \brief true if auto delete file,when close() be invoked
		 */
		bool auto_delete_;

		/**
		 * \brief ref of log
		 */
		int ref_;

		/**
		 * \brief ref locker
		 */
		acl::locker locker_;
	};

	
}