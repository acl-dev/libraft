#pragma once

namespace raft
{
	//todo add log cache
	typedef std::map<log_index_t, log_index_t> log_infos_t;

	typedef typename std::map<log_index_t, 
		log_index_t>::iterator log_infos_iter_t;

	class log_manager
	{
	public:
		log_manager(const std::string &path);
		
		virtual ~log_manager();

		bool reload_logs();

		log_index_t write(const log_entry &entry);
		
		bool read(log_index_t index, int max_bytes,int max_count,
			std::vector<log_entry*> &entries);

		bool read(log_index_t index, log_entry &entry);
		
		void truncate(log_index_t index);

		size_t log_count();
		
		log_index_t start_index();
		
		log_index_t last_index();

		term_t last_term();

		/**
		 * return all logs info .
		 * one log info is [start_log_index, last_log_index].
		 * @return
		 */
		log_infos_t logs_info();

		int discard_log(log_index_t log_start_index);

		void set_log_size(size_t log_size);

		void set_last_index(log_index_t index);

		void set_last_term(term_t term);
	protected:

		virtual log *create(const std::string &file_path) = 0;

		log *find_log(log_index_t index);

		std::string		path_;
		size_t			log_size_;
		log_index_t		last_index_;
		term_t			last_term_;
		acl::locker		locker_;
		log				*last_log_;
		std::map<log_index_t, log*> logs_;
	};
}