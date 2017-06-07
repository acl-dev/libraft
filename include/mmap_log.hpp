#pragma once
namespace raft
{
	class mmap_log : public log
	{
	public:
		mmap_log(log_index_t pre_index, int file_size);

		virtual bool open(const std::string &filename);

		virtual log_index_t write(const log_entry & entry);

		virtual bool truncate(log_index_t index);

		virtual bool read(log_index_t index, log_entry &entry);

		virtual bool read(log_index_t index,
			int max_bytes,
			int max_count,
			std::vector<log_entry> &entries,
			int &bytes);

		virtual bool eof();

		virtual bool empty();

		virtual log_index_t last_index();

		virtual log_index_t start_index();

		virtual std::string file_path();

	private:
		~mmap_log();

		virtual void close();

		static bool get_entry(unsigned char *& buffer, log_entry &entry);

		unsigned char* get_data_buffer(log_index_t index);

		size_t max_index_size(size_t max_mmap_size) const;

		static size_t one_index_size();

		bool reload_log();

		bool set_data_wbuf(log_index_t index);

		void reload_start_index();

		unsigned char* get_index_buffer(log_index_t index);

		static void *open_mmap(ACL_FILE_HANDLE fd, size_t maxlen);

		static void close_mmap(void *map);

		bool is_open_;
		bool eof_;

		std::string data_filepath_;
		std::string index_filepath_;
		log_index_t start_index_;
		log_index_t last_index_;
		acl::locker write_locker_;

		size_t data_buf_size_;
		size_t index_buf_size_;

		unsigned char *data_buf_;
		unsigned char *data_wbuf_;

		unsigned char *index_buf_;
		unsigned char *index_wbuf_;
	};


	class mmap_log_manager : public log_manager
	{
	public:
		mmap_log_manager(const std::string &log_path);

	private:
		/**
		* \brief create mmap_log obj
		* \param filepath mmap_log
		* \return mmap_log if ok.or NULL
		*/
		virtual log *create(const std::string &filepath) override;
	};
}