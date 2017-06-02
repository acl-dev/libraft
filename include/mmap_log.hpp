#pragma once
namespace raft
{
#define __MAGIC_NUM__ 244641312

	class mmap_log : public log
	{
	public:

		mmap_log()
		{
			max_size = 1024 * 1024 *512;
			log_start_index_ = 0;
		}
		virtual bool open(const std::string &filename)
		{
			
			int init_size = 1024 * 1024 * 8;

			ACL_FILE_HANDLE fd = acl_file_open(filename.c_str(),O_RDWR | O_CREAT, 0600);
			if (fd == ACL_FILE_INVALID) 
			{
				logger_error("open %s error %s\r\n", filename.c_str(), acl_last_serror());
				return false;
			}

			data_buf_ = data_wbuf_ = (unsigned char*)open_mmap(fd, max_size);
			if (data_buf_ == NULL)
			{
				logger_error("open_mmap %s error %s\r\n", filename.c_str(),
					acl_last_serror());
				acl_file_close(fd);
				return false;
			}
			acl_file_close(fd);

			std::string indexfile = filename + ".index";
			fd = acl_file_open(indexfile.c_str(), O_RDWR | O_CREAT, 0600);
			if (fd == ACL_FILE_INVALID)
			{
				logger_error("open %s error %s\r\n", indexfile.c_str(), acl_last_serror());
				close();
				return false;
			}
			index_buf_ = index_wbuf_= (unsigned char*)open_mmap(fd, max_size);

			if (index_buf_ == NULL)
			{
				logger_error("acl_vstring_mmap_alloc %s error %s\r\n", indexfile.c_str(),
					acl_last_serror());
				close();
				acl_file_close(fd);
				return false;
			}
			acl_file_close(fd);
			return true;
		}
		
		virtual bool close()
		{

		}


		virtual bool write(const log_entry & entry)
		{
			acl::lock_guard lg(write_locker_);

			size_t offset = (data_wbuf_ - data_buf_);
			size_t remail_len = max_size -  offset - sizeof(unsigned int);
			size_t entry_len = get_sizeof(entry);

			if (remail_len  < entry_len)
			{
				logger_error("mmap_log reach max_len");
				return false;
			}

			put_uint32(data_wbuf_, __MAGIC_NUM__);
			put_message(data_wbuf_, entry);

			put_uint32(index_wbuf_, __MAGIC_NUM__);
			put_uint64(index_wbuf_, entry.index());
			put_uint32(index_wbuf_, offset);

			return true;
		}
		
		virtual bool truncate_suffix(log_entry_index_t index)
		{
			unsigned char *index_pbuf = compute_index(index);
			if (!index_pbuf)
				return true;


		}
		
		virtual bool get_log_entry(log_entry_index_t index, log_entry &entry)
		{
			unsigned char *index_pbuf = compute_index(index);
			if (!index_pbuf)
				return false;

			unsigned int value = get_uint32(index_pbuf);
			if (value != __MAGIC_NUM__)
				return false;

			log_entry_index_t index_value = get_uint64(index_pbuf);
			if (index_value != index)
			{
				logger_fatal("mmap_log error");
				return false;
			}

			unsigned int offset = get_uint32(index_pbuf);
			unsigned char *data_buf = data_buf_ + offset;

			return get_message(data_buf, entry);

		}

		virtual bool get_log_entries(
			log_entry_index_t index, 
			int max_bytes, 
			int max_count, 
			std::vector<log_entry> &)
		{
			return false;
		}

		virtual bool should_checkpoint()
		{

		}
	private:
		unsigned char*compute_index(log_entry_index_t index)
		{
			if (log_start_index_ == 0)
			{
				unsigned char *buffer_ptr = index_buf_;
				unsigned int value = get_uint32(buffer_ptr);
				if (value == __MAGIC_NUM__)
				{
					log_start_index_ = get_uint64(buffer_ptr);
				}
			}

			if (index < log_start_index_)
				return NULL;
			else if (index == log_start_index_)
				return index_buf_;

			unsigned char *buffer = index_buf_;
			buffer += (index - log_start_index_) * 
				(sizeof(log_entry_index_t) + sizeof(int) + sizeof(int));

			return buffer;
		}

		void *open_mmap(ACL_FILE_HANDLE fd, size_t maxlen)
		{
			void *data = NULL;
			ACL_FILE_HANDLE hmap;
#ifdef ACL_UNIX
			data = mmap(NULL, maxlen, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
			if (data == MAP_FAILED)
				acl_msg_fatal("mmap error: %s", acl_last_serror());
#elif defined(_WIN32) || defined(_WIN64)
			hmap = CreateFileMapping(fd, NULL, PAGE_READWRITE, 0,maxlen, NULL);
			if (hmap == NULL)
				acl_msg_fatal("CreateFileMapping: %s", acl_last_serror());
			data = (unsigned char *)MapViewOfFile(hmap,FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0);
			if (data == NULL)
				acl_msg_fatal("MapViewOfFile error: %s", acl_last_serror());
#else
			acl_msg_fatal("%s: not supported yet!", __FUNCTION__);
#endif
			return data;
		}

		log_entry_index_t log_start_index_;
		acl::locker write_locker_;

		int max_size;
		unsigned char *data_buf_;
		unsigned char *data_wbuf_;

		unsigned char *index_buf_;
		unsigned char *index_wbuf_;
	};
}