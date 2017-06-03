#pragma once
namespace raft
{
	struct log 
	{
		virtual ~log() = 0;

		virtual bool open(const std::string &filepath) = 0;
		
		virtual bool close() = 0;
		
		virtual bool write(const log_entry &) = 0;
		
		virtual bool truncate(log_index_t) = 0;
		
		virtual bool read(log_index_t, log_entry &) = 0;

		virtual bool read(log_index_t index, int max_bytes, 
			int max_count, std::vector<log_entry> &, int &bytes) = 0;

		virtual log_index_t last_index() = 0;

		virtual log_index_t start_index() = 0;
		
		virtual bool eof() = 0;
	};
}