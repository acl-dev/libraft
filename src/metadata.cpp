#include<string>
#include "raft.hpp"

#define __METADATA_EXT__ ".meta"

#ifndef __MAGIC_START__
#define __MAGIC_START__ 123456789
#endif // __MAGIC_START__ 

#ifndef __MAGIC_END__
#define __MAGIC_END__ 987654321
#endif // __MAGIC_END__

#define APPLIED_INDEX	1
#define COMMITTED_INDEX 2
#define VOTE_FOR		3
#define CURRENT_TERM	4

#define CURRENT_TERM_LEN    \
(sizeof(int) * 2 + sizeof(term_t) + sizeof(char))

#define COMMITTED_INDEX_LEN \
(sizeof(int) * 2 + sizeof(log_index_t) + sizeof(char))

#define APPLIED_INDEX_LEN	\
(sizeof(int) * 2 + sizeof(log_index_t) + sizeof(char))

#define METADATA_SECTION 102
/*
metafile format:

|__MAGIC_START__|char{value type}|value|char{id of value}|__MAGIC_START__|


*/
namespace raft
{

	metadata::metadata()
	{
		current_term_ = 0;
		committed_index_ = 0;
		applied_index_ = 0;

		max_buffer_size_ = 10*1024*1024;
		write_pos_ = 0;
		buf_ = 0;
		file_index_ = 0;
	}

	bool metadata::reload(const std::string &path)
	{

		std::set<std::string> files = 
			list_dir(path, __METADATA_EXT__);

		if (files.empty())
		{
			acl::string file;
			file_index_++;
			file.format("%s%llu%s",
				path.c_str(), 
				file_index_, 
				__METADATA_EXT__);

			if (open(file.c_str(), true))
			{
				return check_point();
			}
			logger_error("create medata file error");
			return false;
		}
			

		for (std::set<std::string>::reverse_iterator it = 
			files.rbegin();it != files.rend(); ++it)
		{
			//reload the new metadata file.
			//the new one in the files last one.
			if (!open(*it, false))
			{
				logger_error("open file error.");
				continue;
			}
			if (reload_file(*it))
			{
				logger("reload_file ok");
				return true;
			}
		}
		return  true;
	}
	bool metadata::reload_file(const std::string &file_path)
	{
		do
		{
			//check if reach end of file.
			if (write_pos_ - buf_ >= max_buffer_size_)
				return true;

			//if has not magic num. it has not data anymore.
			if (get_uint32(write_pos_) != __MAGIC_START__)
				break;

			char ch = get_uint8(write_pos_);
			switch (ch)
			{
			case COMMITTED_INDEX:
			{
				if (load_committed_index())
					continue;
				return false;
			}
			case APPLIED_INDEX:
			{
				if (load_applied_index())
					continue;
				return false;
			}
			case VOTE_FOR:
			{
				if (load_vote_for())
					continue;
				return false;
			}
			case CURRENT_TERM:
			{
				if (load_current_term())
					continue;
				return false;
			}
			default:
				break;
			}
		} while (true);
	}
	bool metadata::load_committed_index()
	{
		committed_index_ = get_uint64(write_pos_);
		if (get_uint32(write_pos_) != __MAGIC_END__)
		{
			logger_error("metadata broken!!!");
			return false;
		}
		return true;
	}
	bool metadata::load_applied_index()
	{
		applied_index_ = get_uint64(write_pos_);
		if (get_uint32(write_pos_) != __MAGIC_END__)
		{
			logger_error("metadata broken!!!");
			return false;
		}
		return true;
	}
	bool metadata::load_current_term()
	{
		applied_index_ = get_uint64(write_pos_);
		if (get_uint32(write_pos_) != __MAGIC_END__)
		{
			logger_error("metadata broken!!!");
			return false;
		}
		return true;
	}
	bool metadata::load_vote_for()
	{
		vote_term_ = get_uint64(write_pos_);
		vote_for_ = get_string(write_pos_);
		if (get_uint32(write_pos_) != __MAGIC_END__)
		{
			logger_error("metadata broken!!!");
			return false;
		}
		return true;
	}
	bool metadata::check_remain_buffer(size_t size)
	{
		//sizeof remail for read __MAGIC_START__
		return write_pos_ - buf_ > size + sizeof(uint32_t);
	}
	bool metadata::set_committed_index(log_index_t index)
	{
		acl::lock_guard lg(locker_);
		if (!check_remain_buffer(COMMITTED_INDEX_LEN))
		{
			logger("metadata end of file");
			return false;
		}
			
		put_uint32(write_pos_, __MAGIC_START__);
		put_uint8(write_pos_, COMMITTED_INDEX);
		put_uint64(write_pos_, index);
		put_uint32(write_pos_, __MAGIC_END__);
		committed_index_ = index;
		return true;
	}

	log_index_t metadata::get_committed_index()
	{
		acl::lock_guard lg(locker_);
		return committed_index_;
	}

	bool metadata::set_applied_index(log_index_t index)
	{
		acl::lock_guard lg(locker_);
		if (!check_remain_buffer(APPLIED_INDEX_LEN))
		{
			logger("metadata end of file");
			return false;
		}

		put_uint32(write_pos_, __MAGIC_START__);
		put_uint8(write_pos_, APPLIED_INDEX);
		put_uint64(write_pos_, index);
		put_uint32(write_pos_, __MAGIC_END__);
		applied_index_ = index;
		return true;
	}

	log_index_t metadata::get_applied_index()
	{
		acl::lock_guard lg(locker_);
		return applied_index_;
	}

	bool metadata::set_current_term(term_t term)
	{
		acl::lock_guard lg(locker_);
		if (!check_remain_buffer(APPLIED_INDEX_LEN))
		{
			logger("metadata end of file");
			return false;
		}

		put_uint32(write_pos_, __MAGIC_START__);
		put_uint8(write_pos_, APPLIED_INDEX);
		put_uint64(write_pos_, term);
		put_uint32(write_pos_, __MAGIC_END__);
		current_term_ = term;
		return true;
	}

	term_t metadata::get_current_term()
	{
		acl::lock_guard lg(locker_);
		return current_term_;
	}

	bool metadata::set_vote_for(const std::string &candidate_id, term_t term)
	{
		acl::lock_guard lg(locker_);

		size_t len = APPLIED_INDEX_LEN + sizeof(int) + candidate_id.size();
		if (!check_remain_buffer(len))
		{
			logger("metadata end of file");
			return false;
		}

		put_uint32(write_pos_, __MAGIC_START__);
		put_uint8(write_pos_, APPLIED_INDEX);
		put_uint64(write_pos_, term);
		put_string(write_pos_, candidate_id);
		put_uint32(write_pos_, __MAGIC_END__);
		vote_for_ = candidate_id;
		vote_term_ = term;
	}

	std::pair<term_t, std::string> metadata::get_vote_for()
	{
		acl::lock_guard lg(locker_);
		return std::make_pair(vote_term_, vote_for_);
	}

	bool metadata::open(const std::string &file_path, bool create)
	{
		long long size = acl_file_size(file_path.c_str());
		//file not exist
		if (size == -1 )
		{
			logger("open file(%s) error:%s", 
				file_path.c_str(),
				acl::last_serror());

			if (!create)
				return false;
			//default max size of metadata file
			size = max_buffer_size_;
		}

		ACL_FILE_HANDLE fd = acl_file_open( 
			file_path.c_str(), O_RDWR | O_CREAT,0600);

		if (fd == ACL_FILE_INVALID)
		{
			logger_error("open file error.%s", acl::last_serror());
			return false;
		}

		//open mmap
		write_pos_ = buf_ = 
			static_cast<unsigned char *>(open_mmap(fd, size));
		
		//close fd.we don't need anymore
		acl_file_close(fd);

		return buf_ != NULL;
	}

	bool metadata::check_point()
	{
		set_applied_index(applied_index_);
		set_committed_index(committed_index_);
		set_current_term(current_term_);
		set_vote_for(vote_for_, vote_term_);
	}
}