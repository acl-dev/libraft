#pragma once
namespace raft
{
	class metadata
	{
	public:
		metadata();

		bool reload(const std::string &path);

		bool set_committed_index(log_index_t index);

		log_index_t get_committed_index();

		bool set_applied_index(log_index_t index);

		log_index_t get_applied_index();

		bool set_current_term(term_t term);

		term_t get_current_term();

		bool set_vote_for(const std::string &candidate_id, term_t term);

		std::pair<term_t,std::string> get_vote_for();

	private:
		bool check_remain_buffer(size_t size);

		bool load_committed_index();

		bool load_applied_index();

		bool load_vote_for();
		
		bool load_current_term();

		void reset();

		bool reload_file(const std::string &file_path);

		bool open(const std::string &file_path, bool create = false);

		bool check_point();

	private:
		unsigned long long file_index_;

		term_t current_term_;
		log_index_t committed_index_;
		log_index_t applied_index_;
		std::string vote_for_;
		term_t vote_term_;

		acl::locker locker_;
		std::string path_;

		size_t max_buffer_size_;
		unsigned char *write_pos_;
		unsigned char *buf_;
	};
}
