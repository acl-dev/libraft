#pragma once
namespace raft
{
	struct log 
	{
		virtual bool open(const std::string &filename) = 0;
		
		virtual bool close() = 0;
		
		virtual bool write(const log_entry &) = 0;
		
		virtual bool truncate(log_entry_index_t ) = 0;
		
		virtual bool get_log_entry(log_entry_index_t, log_entry &) = 0;

		virtual bool get_log_entries(log_entry_index_t index, 
			int max_bytes, int max_count, std::vector<log_entry> &) = 0;

		virtual bool eof() = 0;
	};
}