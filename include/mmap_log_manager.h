#pragma once
namespace raft
{
	class mmap_log_manager: public log_manager
	{
	public:
		mmap_log_manager(const std::string &log_path);

		~mmap_log_manager();

	private:
		virtual log *create(const std::string &filepath);

		virtual void release_log(log *&_log);

		virtual bool destroy_log(log *_log);
	};
}