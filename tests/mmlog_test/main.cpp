#include "raft.hpp"

using namespace raft;

int count = 1000;


void test_read(raft::mmap_log &log)
{
	acl_assert(log.start_index() == 1);
	acl_assert(log.last_index() == 999);;


	for (int i = 1; i < count; i++)
	{
		log_entry entry;
		log.read(i, entry);
		acl_assert(entry.index() == i);
		acl_assert(entry.log_data() == "hello");
	}

	acl_assert(log.last_index() == count - 1);

	std::vector<log_entry> entries;
	int bytes = 0;
	log.read(1, 1000000, 100000, entries, bytes);

	acl_assert(entries.size() == 999);

}
void test_write(mmap_log &log)
{
	for (int i = 1; i < count; i++)
	{
		log_entry entry;
		entry.set_index(i);
		entry.set_term(i);
		entry.set_type(log_entry_type::e_raft_log);
		entry.set_log_data(std::string("hello"));

		acl_assert(log.write(entry));
	}
}
int main()
{
	mmap_log log;
	const char *filepath = "mmap.log";
	
	bool exist = acl_file_size(filepath) != -1;
	acl_assert(log.open(filepath));

	if (exist)
	{
		test_read(log);
	}
	else
	{
		test_write(log);
	}
	acl_assert(log.close());

	return 0;
}