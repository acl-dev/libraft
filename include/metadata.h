#pragma once

class metadata
{
public:
    metadata();

    bool reload(const std::string &path);

    bool set_committed_index(log_index_t index);

    bool get_committed_index(log_index_t &index);

    bool set_applied_index(log_index_t index);

    bool get_applied_index(log_index_t &index);

    bool set_current_term(term_t term);

    bool get_current_term(term_t& term);

    bool set_vote_for(const std::string &candidate_id, term_t term);

    bool get_vote_for(std::string &candidate_id, term_t &term);

private:

    void open(const std::string &file_path);

    void check_point();

private:
    acl::locker locker_;
    std::string path_;

    size_t max_buffer_size_;
    unsigned char *mmap_buffer_;
};