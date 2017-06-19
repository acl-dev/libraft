#include "raft.hpp"
using namespace raft;

#define METADATA_TEST_COUNT 100000

raft::metadata metadata_;

void do_write_test(size_t count)
{
	for (size_t i = 0; i < count; i++)
	{
		metadata_.set_applied_index(i);
		metadata_.set_committed_index(i);
		metadata_.set_current_term(i);
		metadata_.set_vote_for("hello", i);
	}
	acl_assert(metadata_.get_applied_index() == count-1);
	acl_assert(metadata_.get_committed_index() == count - 1);
	acl_assert(metadata_.get_current_term() == count - 1);
	std::pair<term_t, std::string> vote_for = metadata_.get_vote_for();
	acl_assert(vote_for.first == count - 1);
	acl_assert(vote_for.second == "hello");
}
void do_read_test(size_t value)
{
	acl_assert(metadata_.get_applied_index() == value);
	acl_assert(metadata_.get_committed_index() == value);
	acl_assert(metadata_.get_current_term() == value);
	std::pair<term_t, std::string> vote_for = metadata_.get_vote_for();
	acl_assert(vote_for.first == value);
	acl_assert(vote_for.second == "hello");
}
int main()
{
	acl_assert(metadata_.reload("metadata_test/"));
	if (metadata_.get_applied_index() == 0)
	{
		do_write_test(METADATA_TEST_COUNT);
	}
	else
	{
		do_read_test(METADATA_TEST_COUNT - 1);
	}

	return 0;
}