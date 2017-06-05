#pragma once
#define __4MB__			4*1024*1024
namespace raft
{
	class mmap_log : public log
	{
	public:
		mmap_log(int max_size = __4MB__);
		~mmap_log();

		virtual bool open(const std::string &filename);

		virtual bool close();

		virtual bool write(const log_entry & entry);

		virtual bool truncate(log_index_t index);

		virtual bool read(log_index_t index, log_entry &entry);

		virtual bool read(log_index_t index,
			int max_bytes,
			int max_count,
			std::vector<log_entry> &entries,
			int &bytes);

		virtual bool eof();

		virtual log_index_t last_index();

		virtual log_index_t start_index();

	private:
		bool get_entry(unsigned char *& buffer, log_entry &entry);

		unsigned char* get_data_buffer(log_index_t index);
		
		size_t max_index_size(size_t max_mmap_size);
		
		size_t one_index_size();
		
		bool reload_log();
		
		bool move_data_wbuf(log_index_t index);
		
		void reload_start_index();
		
		unsigned char* get_index_buffer(log_index_t index);

		void *open_mmap(ACL_FILE_HANDLE fd, size_t maxlen);

		void close_mmap(void *map);

		bool is_open_;
		bool eof_;

		log_index_t start_index_;
		log_index_t last_index_;
		acl::locker write_locker_;

		acl_pthread_rwlock_t rwlocker_;

		size_t max_data_size_;
		size_t max_index_size_;

		unsigned char *data_buf_;
		unsigned char *data_wbuf_;

		unsigned char *index_buf_;
		unsigned char *index_wbuf_;
	};
}