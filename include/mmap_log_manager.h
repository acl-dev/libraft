#pragma once
namespace raft
{
	class mmap_log_manager: public log_manager
	{
	public:
		mmap_log_manager(const std::string &log_path);

	private:
		virtual log *create(const std::string &filepath);
	};
}