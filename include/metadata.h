#pragma once
namespace raft
{

	class metadata
	{
	public:
        /**
         * metadata construct.
         * @param max_file_size when file full.
         * it will create new one
         * if max_file_size too small. it create so many file
         * (it will auto delete old file).if max_file_size too big
         * it will take too long time to reload metadata.
         */
		metadata(size_t max_file_size = 8*1024*1024);

        ~metadata ();

        /**
         * reload metadata
         * @param path the path to reload metadata
         * @return return true if reload ok.
         * return false some error  happen
         */
		bool reload(const std::string &path);

		bool set_committed_index(log_index_t index);

		log_index_t get_committed_index();

		bool set_applied_index(log_index_t index);

		log_index_t get_applied_index();

		bool set_current_term(term_t term);

		term_t get_current_term();

		bool set_vote_for(const std::string &id, term_t term);

		std::pair<term_t,std::string> get_vote_for();

		bool set_peer_infos(const std::vector<peer_info> &infos);

		std::vector<peer_info> get_peer_info();

        void print_status();
	private:
        bool create_new_file ();

		bool check_remain_buffer(size_t size);

		bool load_committed_index();

		bool load_applied_index();

		bool load_vote_for();
		
		bool load_current_term();

        bool load_peer_info();

		bool reload();

		bool open(const std::string &file_path, bool create);

		bool check_point();

	private:
		size_t file_index_;

		term_t current_term_;
		log_index_t committed_index_;
		log_index_t applied_index_;
		std::string vote_for_;
		term_t vote_term_;

		std::vector<peer_info> peer_infos_;
		acl::locker locker_;
		std::string path_;
        std::string file_path_;

		size_t max_buffer_size_;
		unsigned char *write_pos_;
		unsigned char *buf_;
	};
}
