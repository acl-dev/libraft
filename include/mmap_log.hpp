#pragma once
namespace raft
{

#define __MAGIC_START__ 123456789
#define __MAGIC_END__   987654321
#define __4MB__			4*1024*1024
#define __64k__			64*1024

class mmap_log : public log
{
public:
	mmap_log(int max_size = __4MB__)
	{
		data_file_size_ = 0;
		index_file_size_ = 0;

		if (max_size < __4MB__)
			max_size = __4MB__;

		while (data_file_size_ < max_size)
			data_file_size_ += __64k__;

		index_file_size_ = compute_max_index_size(data_file_size_);

		start_log_index_ = 0;
		last_log_index_  = 0;
		eof_ = false;
		is_open_ = false;
	}

	virtual bool open(const std::string &filename)
	{
		ACL_FILE_HANDLE fd = acl_file_open(
			filename.c_str(),
			O_RDWR | O_CREAT,
			0600);

		if (fd == ACL_FILE_INVALID) 
		{
			logger_error("open %s error %s\r\n", 
				filename.c_str(), 
				acl_last_serror());
			return false;
		}

		data_buf_ = data_wbuf_ = 
			(unsigned char*)open_mmap(fd, data_file_size_);

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
			logger_error("open %s error %s\r\n", 
				indexfile.c_str(), acl_last_serror());
			close();
			return false;
		}
		index_buf_ = index_wbuf_= (unsigned char*)open_mmap(
			fd, 
			index_file_size_);

		if (index_buf_ == NULL)
		{
			logger_error("acl_vstring_mmap_alloc %s error %s\r\n", 
				indexfile.c_str(),acl_last_serror());

			close();
			acl_file_close(fd);
			return false;
		}
		acl_file_close(fd);
		if (!reload_log())
		{
			logger_error("reload log failed");
			return false;
		}
		is_open_ = true;
		return true;
	}
		
	virtual bool close()
	{
		acl_assert(is_open_);
		if (data_buf_)
			close_mmap(data_buf_);
		if (index_buf_)
			close_mmap(index_buf_);

		data_buf_ = data_wbuf_ = NULL;
		index_buf_ = index_wbuf_ = NULL;
		is_open_ = false;

		return true;
	}


	virtual bool write(const log_entry & entry)
	{
		acl::lock_guard lg(write_locker_);
		
		acl_assert(is_open_);

		
		size_t offset = (data_wbuf_ - data_buf_);
		size_t remail_len = data_file_size_ - offset;
		size_t entry_len = get_sizeof(entry);

		//for __MAGIC_START__, __MAGIC_END__ space
		if (remail_len - sizeof(unsigned int) * 2 < entry_len)
		{
			eof_ = true;
			return false;
		}
		
		if (entry.index() < last_log_index_)
		{
			logger_fatal("log_entry error");
			return false;
		}

		put_uint32(data_wbuf_, __MAGIC_START__);
		put_message(data_wbuf_, entry);
		put_uint32(data_wbuf_, __MAGIC_END__);

		unsigned char *index_buffer = index_wbuf_;

		put_uint32(index_wbuf_, __MAGIC_START__);
		put_uint64(index_wbuf_, entry.index());
		put_uint32(index_wbuf_, (unsigned int)offset);
		put_uint32(index_wbuf_, __MAGIC_END__);

		last_log_index_ = entry.index();
		return true;
	}
		
	virtual bool truncate(log_entry_index_t index)
	{
		acl_assert(is_open_);

		if (index < start_log_index_ || index > last_log_index_)
		{
			logger_error("index error");
			return false;
		}

		unsigned char *index_buffer = compute_index_buffer(index);
		unsigned char *buffer = index_buffer;

		if (!index_buffer || get_uint32(index_buffer))
			return false;
		//get index
		if (get_uint64(index_buffer) != index)
		{
			logger_fatal("mmap_log error");
			return false;
		}
		//get offset
		get_uint32(index_buffer);
		if (get_uint32(index_buffer) == __MAGIC_END__)
		{
			logger_fatal("mmap_log error");
			return false;
		}

		index_wbuf_ = buffer;

		//write 0 to truncate
		put_uint32(buffer, 0);

		last_log_index_ = index - 1;
		
		//update index
		if (start_log_index_ < last_log_index_)
			start_log_index_ = last_log_index_;

		return move_data_wbuf(last_log_index_);
	}
		
	virtual bool get_log_entry(log_entry_index_t index, log_entry &entry)
	{

		unsigned int offset = 0;

		acl_assert(is_open_);

		unsigned char *data_buf = get_data_buffer(index);
		if (!data_buf || get_uint32(data_buf) != __MAGIC_START__)
			return false;

		bool rc = get_message(data_buf, entry);

		if (get_uint32(data_buf) != __MAGIC_END__)
		{
			logger_fatal("mmap error");
			return false;
		}

		return rc;
	}

	virtual bool get_log_entries(
		log_entry_index_t index, 
		int max_bytes, 
		int max_count, 
		std::vector<log_entry> &entries)
	{
		unsigned char *data_buf = get_data_buffer(index);

		if (!data_buf)
			return false;

		while (true)
		{
			log_entry entry;
			if (get_uint32(data_buf) != __MAGIC_START__)
				break;
			if (!get_message(data_buf, entry))
			{
				logger_error("get_message failed");
				break;
			}
			if (get_uint32(data_buf) != __MAGIC_END__)
				break;
			max_bytes -= (int)get_sizeof(entry);
			--max_count ;
			if (max_bytes < 0 || max_bytes <= 0)
				break;
			entries.push_back(entry);
		}
		return !!entries.size();
	}

	virtual bool eof()
	{
		return eof_;
	}

	log_entry_index_t get_last_log_index()
	{
		acl::lock_guard lg(write_locker_);

		return last_log_index_;
	}

	log_entry_index_t get_start_log_index()
	{
		if (start_log_index_ == 0)
			reload_start_log_index();

		return start_log_index_;
	}

private:
	unsigned char* get_data_buffer(log_entry_index_t index)
	{
		unsigned int offset = 0;
		unsigned char *index_buf = compute_index_buffer(index);
		if (!index_buf)
			return false;

		unsigned int magic = get_uint32(index_buf);
		if (magic != __MAGIC_START__)
			return false;

		log_entry_index_t index_value = get_uint64(index_buf);
		if (index_value != index)
		{
			logger_fatal("mmap_log error");
			return false;
		}

		offset = get_uint32(index_buf);
		if (get_uint32(index_buf) != __MAGIC_END__)
		{
			logger_fatal("mmap_log error");
			return false;
		}

		acl_assert(offset < data_file_size_);
		return data_buf_ + offset;
	}
	int compute_max_index_size(int max_mmap_size)
	{
		log_entry entry;
		entry.set_index(-1);
		entry.set_term(-1);
		entry.set_type(log_entry_type::e_raft_log);
		entry.set_log_data(std::string(" "));

		size_t one_entry_len = get_sizeof(entry) + sizeof(int) *2;

		size_t one_index_len = sizeof(long long) + sizeof(int) *3;

		int size = ((max_mmap_size / one_entry_len + 1)* one_index_len);

		int max_size = __64k__;

		while(max_size < size)
		{
			max_size += __64k__;
		}

		return max_size;
	}
	size_t one_index_size()
	{
		return sizeof(long long) + sizeof(int) + sizeof(int);
	}
	bool reload_log()
	{
		if (get_uint32(index_wbuf_) == __MAGIC_START__)
		{
			//get index
			last_log_index_ = start_log_index_ = get_uint64(index_wbuf_);
			//get offset
			get_uint32(index_wbuf_);
			if (get_uint32(index_wbuf_) != __MAGIC_END__)
			{
				logger_error("mmap_error");
				return false;
			}

			size_t max_size = index_file_size_ - sizeof(int);
			
			while(index_wbuf_ - index_buf_< max_size)
			{
				if (get_uint32(index_wbuf_) == __MAGIC_START__)
				{
					//index
					last_log_index_ = get_uint64(index_wbuf_);
					//offset
					get_uint32(index_wbuf_);
					if (get_uint32(index_wbuf_) != __MAGIC_END__)
					{
						logger_fatal("mmap index error");
						return false;
					}
					continue;
				}
				//for get_uint32
				index_wbuf_ -= sizeof(unsigned int);
				break;
			}
		}
		else
		{
			//for get_uint32
			index_wbuf_ -= sizeof(unsigned int);
			return true;
		}
		return move_data_wbuf(last_log_index_);
	}
	bool move_data_wbuf(log_entry_index_t index)
	{
		//index ==0 for empty
		if (index == 0)
		{
			data_wbuf_ = data_buf_;
			return true;
		}

		log_entry entry;
		unsigned char *data_buffer = get_data_buffer(index);

		if (!data_buffer || get_uint32(data_buffer) != __MAGIC_START__)
		{
			logger_fatal("mmap error");
			return false;
		}

		bool rc = get_message(data_buffer, entry);

		if (get_uint32(data_buffer) != __MAGIC_END__)
		{
			logger_fatal("mmap error");
			return false;
		}
		//set data_wbuf to end
		data_wbuf_ = data_buffer;
		return true;
	}
	void reload_start_log_index()
	{
		unsigned char *buffer_ptr = index_buf_;
		if (get_uint32(buffer_ptr) == __MAGIC_START__)
		{
			start_log_index_ = get_uint64(buffer_ptr);
			//get offset
			get_uint32(buffer_ptr);
			if (get_uint32(buffer_ptr) != __MAGIC_END__)
			{
				logger_fatal("mmap_error");
			}
		}
	}
	unsigned char* compute_index_buffer(log_entry_index_t index)
	{
		if (start_log_index_ == 0)
			reload_start_log_index();

		if (index < start_log_index_)
			return NULL;
		else if (!start_log_index_ || index == start_log_index_)
			return index_buf_;

		unsigned char *buffer = index_buf_;
		buffer += (index - start_log_index_) * 
			(sizeof(log_entry_index_t) + sizeof(int) * 3);

		return buffer;
	}

	void *open_mmap(ACL_FILE_HANDLE fd, size_t maxlen)
	{
		void *data = NULL;
		ACL_FILE_HANDLE hmap;

#ifdef ACL_UNIX
		
		data = mmap(
			NULL, 
			maxlen, 
			PROT_READ | PROT_WRITE, 
			MAP_SHARED, 
			fd, 
			0);

		if (data == MAP_FAILED)
			logger_error("mmap error: %s", acl_last_serror());

#elif defined(_WIN32) || defined(_WIN64)

		hmap = CreateFileMapping(
			fd, 
			NULL, 
			PAGE_READWRITE, 
			0,
			(DWORD)maxlen, 
			NULL);

		if (hmap == NULL)
			logger_error("CreateFileMapping: %s", acl_last_serror());

		data = MapViewOfFile(
			hmap, 
			FILE_MAP_READ | FILE_MAP_WRITE, 
			0, 
			0, 
			0);

		if (data == NULL)
			logger_error("MapViewOfFile error: %s",
				acl_last_serror());
#else
		logger_error("%s: not supported yet!", __FUNCTION__);
#endif
		return data;
	}

	void close_mmap(void *map)
	{
#if defined (_WIN32) || defined(_WIN64)
		FlushViewOfFile(map, 0);
		UnmapViewOfFile(map);
#elif defined (ACL_UNIX)
		unmap(map, index_file_size_);
#endif
	}

	bool is_open_;
	bool eof_;

	log_entry_index_t start_log_index_;
	log_entry_index_t last_log_index_;
	acl::locker write_locker_;

	size_t data_file_size_;
	size_t index_file_size_;

	unsigned char *data_buf_;
	unsigned char *data_wbuf_;

	unsigned char *index_buf_;
	unsigned char *index_wbuf_;
};
}