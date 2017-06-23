#pragma once
namespace raft
{
	typedef unsigned long long log_index_t;
	typedef unsigned long long term_t;

	inline size_t get_sizeof(int)
	{
		return sizeof(int);
	}

	inline void put_bool(unsigned char *&buffer_, bool value)
	{
		*buffer_ = value ? 1 : 0;
		buffer_ += sizeof(char);
	}

	inline bool get_bool(unsigned char *&buffer_)
	{
		unsigned char value = buffer_[0];
		buffer_ += sizeof(value);
		return value > 0;
	}

	inline void put_uint8(unsigned char *&buffer_, unsigned char value)
	{
		*buffer_ = value;
		buffer_ += sizeof(value);
	}

	inline unsigned char get_uint8(unsigned char *&buffer_)
	{
		unsigned char value = buffer_[0];
		buffer_ += sizeof(value);
		return value;
	}

	inline void put_uint16(unsigned char *buffer_, unsigned short value)
	{
		buffer_[0] = (unsigned char)(((value) >> 8) & 0xff);
		buffer_[1] = (unsigned char)(value & 0xff);
		buffer_ += sizeof(value);
	}

	inline unsigned short get_uint16(unsigned char *&buffer_)
	{
		unsigned short value = (((unsigned short)buffer_[0]) << 8) | 
			((unsigned short)buffer_[1]);

		buffer_ += sizeof(value);
		return value;
	}

	inline void put_uint32(unsigned char *&buffer_, unsigned int value)
	{
		buffer_[0] = (unsigned char)(((value) >> 24) & 0xff);
		buffer_[1] = (unsigned char)(((value) >> 16) & 0xff);
		buffer_[2] = (unsigned char)(((value) >> 8) & 0xff);
		buffer_[3] = (unsigned char)(value & 0xff);
		buffer_ += sizeof(value);
	}

	inline unsigned int get_uint32(unsigned char *&buffer_)
	{
		unsigned int value =
			(((unsigned int)buffer_[0]) << 24) |
			(((unsigned int)buffer_[1]) << 16) |
			(((unsigned int)buffer_[2]) << 8) |
			((unsigned int)buffer_[3]);
		buffer_ += sizeof(value);
		return value;
	}

	inline void put_uint64(unsigned char *&buffer_, unsigned long long  value)
	{
		buffer_[0] = (unsigned char)(((value) >> 56) & 0xff);
		buffer_[1] = (unsigned char)(((value) >> 48) & 0xff);
		buffer_[2] = (unsigned char)(((value) >> 40) & 0xff);
		buffer_[3] = (unsigned char)(((value) >> 32) & 0xff);
		buffer_[4] = (unsigned char)(((value) >> 24) & 0xff);
		buffer_[5] = (unsigned char)(((value) >> 16) & 0xff);
		buffer_[6] = (unsigned char)(((value) >> 8) & 0xff);
		buffer_[7] = (unsigned char)(value & 0xff);
		buffer_ += sizeof(value);
	}

	inline unsigned long long  get_uint64(unsigned char *&buffer_)
	{
		unsigned long long  value =
			((((unsigned long long )buffer_[0]) << 56) |
			(((unsigned long long )buffer_[1]) << 48) |
				(((unsigned long long )buffer_[2]) << 40) |
				(((unsigned long long )buffer_[3]) << 32) |
				(((unsigned long long )buffer_[4]) << 24) |
				(((unsigned long long )buffer_[5]) << 16) |
				(((unsigned long long )buffer_[6]) << 8) |
				((unsigned long long )buffer_[7]));
		buffer_ += sizeof(value);
		return value;
	}
	inline void put_string(unsigned char *&buffer_, const std::string &str)
	{
		put_uint32(buffer_, (unsigned int)str.size());
		memcpy(buffer_, str.data(), str.size());
		buffer_ += str.size();
	}


	inline std::string get_string(unsigned char *&buffer_)
	{
		unsigned int len = get_uint32(buffer_);
		std::string result((char*)buffer_, len);
		buffer_ += len;
		return result;
	}


	inline void put_message(unsigned char *&buffer_,
							const google::protobuf::Message &msg)
	{
		size_t len = msg.ByteSizeLong();
		put_uint32(buffer_, static_cast<unsigned int>(len + sizeof(int)));
		acl_assert(msg.SerializeToArray(buffer_, static_cast<int>(len)));
		buffer_ += len;
	}
	
	inline bool get_message(unsigned char *&buffer_,
							google::protobuf::Message &entry)
	{
		unsigned int len = get_uint32(buffer_) - sizeof(int);
		bool rc = entry.ParseFromArray(buffer_, static_cast<int>(len));
		buffer_ += len;
		return rc;
	}

	//helper function for write read snapshot
	inline bool write(acl::ostream &_stream, unsigned int value)
	{
		unsigned char len[sizeof(int)];
		unsigned char *plen = len;

		put_uint32(plen, value);
		return _stream.write(len, sizeof(int)) == sizeof(int);
	}
	inline bool write(acl::ostream &_stream, const std::string &data)
	{
		unsigned int len = static_cast<unsigned int>(data.size());

		if (!write(_stream,len))
			return false;
		//empty data.
		if (data.size() == 0)
			return true;

		return _stream.write(data.c_str(), data.size()) == 
			static_cast<int>(data.size());
	}
	
	inline bool read(acl::istream &_stream, unsigned int &value)
	{
		unsigned char len[sizeof(int)];
		unsigned char *plen = len;

		if (_stream.read(len, sizeof(int)) != sizeof(int))
			return false;
		value = get_uint32(plen);
		return true;
	}
	
	inline bool read(acl::istream &_stream, std::string &buffer)
	{
		unsigned int size = 0;
		if (!read(_stream, size))
			return false;
		//empty string
		if (size == 0)
			return true;
		buffer.resize(size);
		return  _stream.read((char*)buffer.data(), size) == size;
	}

	//snapshot compare
	inline bool operator == (const snapshot_info &left,
		const snapshot_info &right)
	{
		return left.last_included_term()
			== right.last_included_term() &&
			left.last_snapshot_index()
			== right.last_snapshot_index();
	}

	inline bool operator != (const snapshot_info &left,
		const snapshot_info &right)
	{
		return !(left == right);
	}
    //mmap

    inline void *open_mmap(ACL_FILE_HANDLE fd, size_t max_len)
    {
        void *data = NULL;

#ifdef ACL_UNIX
        if(ftruncate(fd, max_len) == -1)
        {
            logger_error("ftruncate error.:%s", acl::last_serror());
            return NULL;
        }

        data = mmap(
                NULL,
                max_len,
                PROT_READ | PROT_WRITE,
                MAP_SHARED,
                fd,
                0);

        if (data == MAP_FAILED)
            logger_error("mmap error: %s", acl_last_serror());

#elif defined(_WIN32) || defined(_WIN64)
        (void)max_len;

        ACL_FILE_HANDLE hmap =
			CreateFileMapping(
				fd,
				NULL,
				PAGE_READWRITE,
				0,
				static_cast<DWORD>(max_len),
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
	//close mmap file;
    inline void close_mmap(void *map, size_t map_size)
    {
#if defined (_WIN32) || defined(_WIN64)
        (void)map_size;
		acl_assert(FlushViewOfFile(map, 0));
		acl_assert(UnmapViewOfFile(map));
#elif defined (linux)
        munmap(map, map_size);
#endif
    }

	inline std::string 
		get_filename(const std::string &file_path)
	{
		std::string file_name;
		size_t pos = file_path.find_last_of('/');
		
		if (pos == std::string::npos)
			pos = file_path.find_last_of('\\');

		if (pos != std::string::npos)
			file_name = file_path.substr(pos + 1);
		else
			file_name = file_path;

		pos = file_name.find_last_of('.');
		if (pos != std::string::npos)
			return file_name.substr(0, pos);
		return file_name;
	}

	/**
	 * list dir files.
	 */
	inline std::set<std::string> 
		list_dir(const std::string &path,
                 const std::string &ext_)
	{
		std::set<std::string> files;
		const char* file_path = NULL;

		acl::scan_dir scan;
		if (!scan.open(path.c_str(), false))
		{
			logger_error("scan open error %s",
                         acl::last_serror());
			return files;
		}
		while ((file_path = scan.next_file(true)))
		{
			if (ext_.size())
			{
				if (acl_strrncasecmp(file_path,
                                     ext_.c_str(),
                                     ext_.size()) == 0)
				{
					files.insert(file_path);
				}
			}
			else
			{
				files.insert(file_path);
			}
			
		}
		return files;
	}

    /**
     * if path is not end with slash('/') or backslash('\\').
     * make it end with slash('/')
     * @param path
     */
    inline void append_slash(std::string &path)
    {
        if (path.size())
        {
            //back
            char ch = path[path.size() - 1];
            if (ch != '/'  && ch != '\\')
            {
                path.push_back('/');
            }
        }
    }
}