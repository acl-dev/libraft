#pragma once
namespace raft
{
	struct log 
	{
		log()
		{
			auto_delete_file_ = false;
			ref_ = 0;
		}
		virtual ~log() {};

		virtual bool open(const std::string &filepath) = 0;

		virtual std::string file_path() = 0;
		
		virtual bool write(const log_entry &) = 0;
		
		virtual bool truncate(log_index_t) = 0;
		
		virtual bool read(log_index_t, log_entry &) = 0;

		virtual bool read(log_index_t index, int max_bytes, 
			int max_count, std::vector<log_entry> &, int &bytes) = 0;

		virtual log_index_t last_index() = 0;

		virtual log_index_t start_index() = 0;
		
		virtual bool eof() = 0;

		virtual void auto_delete(bool val)
		{
			auto_delete_file_ = val;
		}

		virtual bool auto_delete()
		{
			return auto_delete_file_;
		}

		void inc_ref()
		{
			acl::lock_guard lg(ref_locker_);
			ref_++;
		}

		void dec_ref()
		{
			int ref;
			ref_locker_.lock();
			ref_--;
			ref = ref_;
			ref_locker_.unlock();

			if (ref == 0)
				close();
		}

		int ref()
		{
			acl::lock_guard lg(ref_locker_);
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

		bool auto_delete_file_;
		//ref
		int ref_;
		acl::locker ref_locker_;
	};

	
}