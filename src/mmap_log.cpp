#include "raft.hpp"

#define __MAGIC_START__ 123456789
#define __MAGIC_END__   987654321
#define __64k__			64*1024

#ifndef __INDEX__EXT__
#define __INDEX__EXT__ ".index"
#endif // __INDEX__EXT__

namespace raft
{

	mmap_log::mmap_log(log_index_t last_index, int file_size)
	{
		data_buf_size_ = 0;
		index_buf_size_ = 0;

		while (data_buf_size_ < file_size)
			data_buf_size_ += __64k__;

		index_buf_size_ = max_index_size(data_buf_size_);

		last_index_ = last_index;
		start_index_ = 0;
		eof_ = false;
		is_open_ = false;
	}

	mmap_log::~mmap_log()
	{
		if (is_open_)
			close();
	}

	bool mmap_log::open(const std::string &filepath)
	{
		data_filepath_ = filepath;

		acl_int64 file_size = acl_file_size(filepath.c_str());

		//not exist
		if (file_size == -1)
			file_size = data_buf_size_;
		else
			data_buf_size_ = file_size;

		ACL_FILE_HANDLE fd = acl_file_open(
			filepath.c_str(),
			O_RDWR | O_CREAT,
			0600);

		if (fd == ACL_FILE_INVALID)
		{
			logger_error("open %s error %s\r\n",
				filepath.c_str(),
				acl_last_serror());
			return false;
		}

		data_buf_ = data_wbuf_ =
			(unsigned char*)open_mmap(fd, data_buf_size_);

		if (!data_buf_)
		{
			logger_error("open_mmap %s error %s\r\n",
				filepath.c_str(),
				acl_last_serror());

			acl_file_close(fd);
			return false;
		}
		acl_file_close(fd);

		index_filepath_ = filepath + __INDEX__EXT__;

		file_size = acl_file_size(index_filepath_.c_str());
		if (file_size == -1)//not exist
			file_size = index_buf_size_;
		else
			index_buf_size_ = file_size;

		fd = acl_file_open(index_filepath_.c_str(),
			O_RDWR | O_CREAT,
			0600);

		if (fd == ACL_FILE_INVALID)
		{
			logger_error("open %s error %s\r\n",
				index_filepath_.c_str(), acl_last_serror());

			close_mmap(data_buf_, data_buf_size_);
			data_buf_ = data_wbuf_ = NULL;
			return false;
		}
		index_buf_ = index_wbuf_ =
			(unsigned char*)open_mmap(fd, file_size);

		acl_file_close(fd);

		if (!index_buf_)
		{
			logger_error("acl_vstring_mmap_alloc"
				" %s error %s\r\n",
				index_filepath_.c_str(),
				acl_last_serror());

			close_mmap(data_buf_, data_buf_size_);
			data_buf_ = data_wbuf_ = NULL;
			return false;
		}

		if (!reload_log())
		{
			logger_error("reload log failed");
			return false;
		}
		is_open_ = true;
		return true;
	}

	void mmap_log::close()
	{
		if (data_buf_)
			close_mmap(data_buf_, data_buf_size_);
		if (index_buf_)
			close_mmap(index_buf_, index_buf_size_);

		if (auto_delete())
		{
			if (remove(data_filepath_.c_str()) != 0)
				logger_error("delete file error log filepath: %s"
					", error str:%s",
					data_filepath_.c_str(),
					acl::last_serror());

			if (remove(index_filepath_.c_str()))
				logger_error("delete file error ,index filepath: %s, "
					"error str:%s",
					index_filepath_.c_str(),
					acl::last_serror());
		}
		is_open_ = false;
	}

	log_index_t mmap_log::write(const log_entry & entry)
	{
		acl::lock_guard lg(write_locker_);

		size_t offset = (data_wbuf_ - data_buf_);
		size_t remail_len = data_buf_size_ - offset;
		size_t entry_len = get_sizeof(entry);
		log_index_t index = last_index_ + 1;

		//for __MAGIC_START__, __MAGIC_END__ space
		if (remail_len < entry_len + sizeof(unsigned int) * 2)
		{
			logger("mmap_log eof");
			eof_ = true;
			return 0;
		}
		log_entry &entry2 = const_cast<log_entry &>(entry);
		entry2.set_index(index);

		put_uint32(data_wbuf_, __MAGIC_START__);
		put_message(data_wbuf_, entry2);
		put_uint32(data_wbuf_, __MAGIC_END__);

		put_uint32(index_wbuf_, __MAGIC_START__);
		put_uint64(index_wbuf_, index);
		put_uint32(index_wbuf_, static_cast<unsigned int>(offset));
		put_uint32(index_wbuf_, __MAGIC_END__);

		//write ok. update last_index_
		last_index_ = index;
		if (start_index_ == 0)
			start_index_ = last_index_;

		return index;
	}

	bool mmap_log::truncate(log_index_t index)
	{
		acl::lock_guard lg(write_locker_);

		if (!is_open_)
		{
			logger("mmap log not open");
			return false;
		}

		if (index < start_index_ || index > last_index_)
		{
			logger_error("index error");
			return false;
		}

		unsigned char *index_buffer = get_index_buffer(index);
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

		last_index_ = index - 1;

		//update index
		if (start_index_ < last_index_)
			start_index_ = last_index_;

		return set_data_wbuf(last_index_);
	}

	bool mmap_log::read(log_index_t index,
		int max_bytes,
		int max_count,
		std::vector<log_entry> &entries,
		int &bytes)
	{
		if (max_bytes <= 0 || max_count <= 0)
		{
			logger_error("param error");
			return false;
		}


		if (index < start_index() || index > last_index())
		{
			logger_error("index error,%d", index);
			return false;
		}

		unsigned char *buffer = get_data_buffer(index);
		if (!buffer)
		{
			return false;
		}

		while (true)
		{
			log_entry entry;

			if (buffer - data_buf_ >= static_cast<int>(
				data_buf_size_ - one_index_size()))
				break;

			if (!get_entry(buffer, entry))
				break;

			max_bytes -= static_cast<int>(entry.ByteSizeLong());
			--max_count;

			if (max_bytes <= 0 || max_count <= 0)
				break;

			entries.push_back(entry);
			bytes += static_cast<int>(entry.ByteSizeLong());

			//read the last one
			if (entry.index() == last_index())
				break;;
		}
		return !!entries.size();
	}

	bool mmap_log::read(log_index_t index, log_entry &entry)
	{
		if (!is_open_)
		{
			logger("mmap log not open");
			return false;
		}

		unsigned char *data_buf = get_data_buffer(index);

		if (!get_entry(data_buf, entry))
		{
			return false;
		}
		return true;
	}

	bool mmap_log::eof()
	{
		return eof_;
	}

	bool mmap_log::empty()
	{
		acl::lock_guard lg(write_locker_);
		return data_wbuf_ == data_buf_;
	}

	raft::log_index_t mmap_log::last_index()
	{
		return last_index_;
	}
	std::string mmap_log::file_path()
	{
		return data_filepath_;
	}
	raft::log_index_t mmap_log::start_index()
	{
		return start_index_;
	}

	bool mmap_log::get_entry(unsigned char *& buffer, log_entry &entry)
	{
		if (get_uint32(buffer) != __MAGIC_START__)
			return false;

		if (!get_message(buffer, entry))
		{
			logger_fatal("mmap error");
			return false;
		}

		if (get_uint32(buffer) != __MAGIC_END__)
		{
			logger_fatal("mmap error");
			return false;
		}
		return true;
	}

	unsigned char* mmap_log::get_data_buffer(log_index_t index)
	{
		unsigned int offset = 0;
		unsigned char *index_buf = get_index_buffer(index);
		if (!index_buf)
			return NULL;

		unsigned int magic = get_uint32(index_buf);
		if (magic != __MAGIC_START__)
			return NULL;

		log_index_t index_value = get_uint64(index_buf);
		if (index_value != index)
		{
			logger_fatal("mmap_log error");
			return NULL;
		}

		offset = get_uint32(index_buf);
		if (get_uint32(index_buf) != __MAGIC_END__)
		{
			logger_fatal("mmap_log error");
			return NULL;
		}

		acl_assert(offset < data_buf_size_);
		return data_buf_ + offset;
	}

	size_t mmap_log::max_index_size(size_t max_mmap_size) const
	{
		log_entry entry;
		entry.set_index(-1);
		entry.set_term(-1);
		entry.set_type(log_entry_type::e_raft_log);
		entry.set_log_data(std::string(" "));

		size_t one_entry_len = get_sizeof(entry) + sizeof(int) * 2;

		size_t one_index_len = sizeof(long long) + sizeof(int) * 3;

		size_t size = (max_mmap_size / one_entry_len + 1)* one_index_len;

		size_t max_size = __64k__;

		while (max_size < size)
			max_size += __64k__;

		return max_size;
	}

	size_t mmap_log::one_index_size()
	{
		return sizeof(log_index_t) + sizeof(int) * 3;
	}

	bool mmap_log::reload_log()
	{
		unsigned int value = get_uint32(index_wbuf_);

		//empty file.
		if (value == 0)
		{
			//for get_uint32
			index_wbuf_ -= sizeof(unsigned int);
			return true;
		}
		else if (value != __MAGIC_START__)
		{
			logger_error("reload_log error.not log file");
			return false;
		}

		//get index
		last_index_ = start_index_ = get_uint64(index_wbuf_);

		//get offset
		get_uint32(index_wbuf_);
		if (get_uint32(index_wbuf_) != __MAGIC_END__)
		{
			logger_error("mmap_error");
			return false;
		}

		size_t max_size = index_buf_size_ - sizeof(int);

		while (index_wbuf_ - index_buf_ < static_cast<int>(max_size))
		{
			value = get_uint32(index_wbuf_);
			//end of index log
			if (value == 0)
			{
				index_wbuf_ -= sizeof(unsigned int);
				return set_data_wbuf(last_index_);

			}
			else if (value == __MAGIC_START__)
			{
				//index
				last_index_ = get_uint64(index_wbuf_);
				//offset
				(void)get_uint32(index_wbuf_);

				if (get_uint32(index_wbuf_) != __MAGIC_END__)
				{
					logger_fatal("mmap index error");
					return false;
				}
				continue;
			}
			else
			{
				logger_fatal("mmap index error");
				return false;
			}
		}
		return false;
	}

	bool mmap_log::set_data_wbuf(log_index_t index)
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

		data_wbuf_ = data_buffer;
		return true;
	}

	void mmap_log::reload_start_index()
	{
		unsigned char *buffer_ptr = index_buf_;

		//read magic 
		if (get_uint32(buffer_ptr) == __MAGIC_START__)
		{
			//read index
			start_index_ = get_uint64(buffer_ptr);

			//get offset
			get_uint32(buffer_ptr);

			//read magic 
			if (get_uint32(buffer_ptr) != __MAGIC_END__)
			{
				logger_fatal("mmap_error");
			}
		}
	}

	unsigned char* mmap_log::get_index_buffer(log_index_t index)
	{
		if (start_index_ == 0)
			reload_start_index();

		if (index < start_index_)
			return NULL;
		else if (!start_index_ || index == start_index_)
			return index_buf_;

		size_t offset = (index - start_index_) * one_index_size();

		if (offset >= index_buf_size_ - one_index_size())
			return NULL;

		return index_buf_ + offset;
	}

	void * mmap_log::open_mmap(ACL_FILE_HANDLE fd, size_t maxlen)
	{
		void *data = NULL;

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

		ACL_FILE_HANDLE hmap =
			CreateFileMapping(
				fd,
				NULL,
				PAGE_READWRITE,
				0,
				static_cast<DWORD>(maxlen),
				NULL);

		if (!hmap)
			logger_error("CreateFileMapping: %s", acl_last_serror());

		data = MapViewOfFile(
			hmap,
			FILE_MAP_READ | FILE_MAP_WRITE,
			0,
			0,
			0);

		if (!data)
			logger_error("MapViewOfFile error: %s",
				acl_last_serror());

		acl_assert(CloseHandle(hmap));
#else
		logger_error("%s: not supported yet!", __FUNCTION__);
#endif
		return data;
	}

	void mmap_log::close_mmap(void *map, size_t map_size)
	{
#if defined (_WIN32) || defined(_WIN64)
		(void)map_size;
		acl_assert(FlushViewOfFile(map, 0));
		acl_assert(UnmapViewOfFile(map));
#elif defined (ACL_UNIX)
		unmap(map, map_size);
#endif
	}

	mmap_log_manager::mmap_log_manager(const std::string &log_path)
		:log_manager(log_path)
	{

	}

	log *mmap_log_manager::create(const std::string &filepath)
	{
		log *_log = new mmap_log(last_index_, log_size_);

		if (!_log->open(filepath))
		{
			logger_error("mmap_log open error,%s",
				filepath.c_str());
			_log->dec_ref();
			return NULL;
		}
		return _log;
	}

}