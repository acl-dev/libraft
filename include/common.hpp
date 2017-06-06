#pragma once
namespace raft
{
	typedef unsigned long long log_index_t;
	typedef unsigned long long term_t;

	inline size_t get_sizeof(int)
	{
		return sizeof(int);
	}


	inline size_t get_sizeof(const google::protobuf::Message &entry)
	{
		//sizeof(int) for len.see put_message
		return entry.ByteSizeLong() + sizeof(int);
	}

	////

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


	inline void put_message(unsigned char *&buffer_, const google::protobuf::Message &msg)
	{
		size_t len = get_sizeof(msg);
		put_uint32(buffer_, (unsigned int)len);
		acl_assert(msg.SerializeToArray(buffer_, (int)len));
		buffer_ += len;
	}
	
	inline bool get_message(unsigned char *&buffer_, google::protobuf::Message &entry)
	{
		unsigned int len = get_uint32(buffer_);
		bool rc = entry.ParseFromArray(buffer_, len);
		buffer_ += len;
		return rc;
	}

	//helper function for write read snapshot

	inline bool write(acl::ostream &_stream, const std::string &data)
	{
		unsigned char len[sizeof(int)];
		unsigned char *plen = len;

		put_uint32(plen, (unsigned int)data.size());
		if (_stream.write(len, sizeof(int)) != sizeof(int))
			return false;
		if (_stream.write(data.c_str(), data.size()) != (int)data.size())
			return false;
		return true;
	}

	inline bool read(acl::istream &_stream, std::string &buffer)
	{
		unsigned char len[sizeof(int)];
		unsigned char *plen = len;
		
		if (_stream.read(len, sizeof(int)) != sizeof(int))
			return false;

		unsigned int size = get_uint32(plen);
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
}