#include "metadata.h"
metadata();

bool metadata::reload(const std::string &path)
{

}

bool metadata::set_committed_index(log_index_t index)
{

}

bool metadata::get_committed_index(log_index_t &index)
{

}

bool metadata::set_applied_index(log_index_t index)
{

}

bool metadata::get_applied_index(log_index_t &index)
{

}

bool metadata::set_current_term(term_t term)
{

}

bool metadata::get_current_term(term_t& term)
{

}

bool metadata::set_vote_for(const std::string &candidate_id, term_t term)
{

}

bool metadata::get_vote_for(std::string &candidate_id, term_t &term)
{

}

void metadata::open(const std::string &file_path)
{

}

void metadata::check_point()
{

}