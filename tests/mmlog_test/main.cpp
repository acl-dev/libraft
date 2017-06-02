#include "raft.hpp"


int main()
{
	using namespace raft;
	mmap_log log;

	acl_assert(log.open("mmap.log"));

	acl_assert(log.get_start_log_index() == 1);
	acl_assert(log.get_last_log_index() == 999);;


	std::vector<log_entry> entries;
	log.get_log_entries(1,1000000, 100000,entries);

	acl_assert(entries.size() == 999);

	return 0;

	/*int count = 1000;

	for (int i = 1; i < count;i++)
	{
		log_entry entry;
		entry.set_index(i);
		entry.set_term(i);
		entry.set_type(log_entry_type::e_raft_log);
		entry.set_log_data(std::string("hello"));

		acl_assert(log.write(entry));
	}

	for (int i = 1; i < count; i++)
	{
		log_entry entry;
		log.get_log_entry(i, entry);
		acl_assert(entry.index() == i);
	}

	acl_assert(log.get_last_log_index() == count - 1);

	acl_assert(log.close());*/

	return 0;
}