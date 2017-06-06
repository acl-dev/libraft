#pragma once

namespace raft
{
	class log_manager
	{
	public:
		log_manager(const std::string &path);
		
		virtual ~log_manager();

		bool write(const log_entry &entry);
		
		bool read(log_index_t index, int max_bytes,int max_count,
			std::vector<log_entry> &entries);

		bool read(log_index_t index, log_entry &entry);
		
		size_t log_count();
		
		log_index_t start_log_index();
		
		std::map<log_index_t, log_index_t> logs_info();

		bool destroy_log(log_index_t log_start_index);

	private:
		virtual log *create(const std::string &filepath) = 0;

		virtual void release_log(log *_log) = 0;

		virtual bool destroy_log(log *_log) = 0;

		log *find_log(log_index_t index);

		bool write_new_log(const log_entry &entry);

		void reload_logs();


		std::string path_;

		acl::locker locker_;
		log *current_wlog_;
		std::map<log_index_t, log*> logs_;
	};
}