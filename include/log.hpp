#pragma once
namespace raft
{
	struct log 
	{
		log()
		{
			auto_delete_ = false;
			ref_ = 1;
		}
		virtual ~log() {};

		virtual bool open(const std::string &filepath) = 0;

		virtual std::string file_path() = 0;
		
		virtual log_index_t write(const log_entry &) = 0;
		
		virtual bool truncate(log_index_t) = 0;
		
		virtual bool read(log_index_t, log_entry &) = 0;

		virtual bool read(log_index_t index, 
						  int max_bytes, 
						  int max_count, 
						  std::vector<log_entry> &entries, 
						  int &bytes) = 0;

		virtual log_index_t last_index() = 0;

		virtual log_index_t start_index() = 0;
		
		virtual bool eof() = 0;

		virtual bool empty() = 0;

		virtual void auto_delete(bool val)
		{
			acl::lock_guard lg(locker_);
			auto_delete_ = val;
		}

		virtual bool auto_delete()
		{
			acl::lock_guard lg(locker_);
			return auto_delete_;
		}

		void inc_ref()
		{
			acl::lock_guard lg(locker_);
			ref_++;
		}

		void dec_ref()
		{
			locker_.lock();
			ref_--;
			int ref = ref_;
			locker_.unlock();

			if (ref == 0)
				close();
		}

		int ref()
		{
			acl::lock_guard lg(locker_);
			return ref_;
		}
	protected:
		virtual void close() = 0;

		struct ref_guard
		{
			ref_guard(log *_log)
				:log_(_log)
			{
				log_->inc_ref();
			}
			~ref_guard()
			{
				log_->dec_ref();
			}
			log *log_;
		};

		bool auto_delete_;
		//ref
		int ref_;
		acl::locker locker_;
	};

	
}