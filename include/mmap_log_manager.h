#pragma once
namespace raft
{
	class mmap_log_manager: public log_manager
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