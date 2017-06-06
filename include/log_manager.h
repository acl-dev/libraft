#pragma once

namespace raft
{
	class log_manager
	{
	public:
		log_manager(const std::string &path);
		
		virtual ~log_manager();

		log_index_t write(const log_entry &entry);
		
		bool read(log_index_t index, int max_bytes,int max_count,
			std::vector<log_entry> &entries);

		bool read(log_index_t index, log_entry &entry);
		
		size_t log_count();
		
		log_index_t start_index();
		
		log_index_t last_index();

		std::map<log_index_t, log_index_t> logs_info();

		bool destroy_log(log_index_t log_start_index);

		void set_log_size(int log_size);
	protected:
		log_index_t last_index_no_lock();

		virtual log *create(const std::string &filepath) = 0;

		log *find_log(log_index_t index);

		void reload_logs();

		std::string path_;
		int log_size_;

		acl::locker locker_;
		log *last_log_;
		std::map<log_index_t, log*> logs_;
	};
}