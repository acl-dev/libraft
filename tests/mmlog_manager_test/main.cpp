#include "raft.hpp"
#include <iostream>


using namespace raft;


log_manager *log_manager_;

void create_log_manger()
{
//    mkdir("mmap_log_manger_test",S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH);
	log_manager_ = 
		new mmap_log_manager("mmap_log_manger_test/");
}
void close_log_manager()
{
	delete log_manager_;
	log_manager_ = NULL;
}

char buffer[64];
char *to_string(int value)
{
	memset(buffer, 0, sizeof(buffer));
	sprintf(buffer, "%d", value);
	return  buffer;
}
void write(int begin, int end)
{
	//write
	for (int i = begin; i < end; i++)
	{
		log_entry entry;
		entry.set_term(1);
		entry.set_type(e_raft_log);

		std::string buffer(1000, 'a');
		buffer += to_string(i);

		entry.mutable_log_data()->append(buffer);
		log_manager_->write(entry);
	}
}
void read(int start, int end)
{
	for (int i = start; i < end ; i++)
	{
		log_entry entry;
		acl_assert(log_manager_->read(i, entry));
		std::cout << entry.log_data().substr(1000)<< std::endl;
	}
}

void read_all()
{
	std::vector<log_entry> entries;
	log_manager_->read(log_manager_->start_index(),
		1024 * 1024 * 1024, 
		log_manager_->last_index(), 
		entries);

	std::cout << 
		"all log entries count:"<<
		entries.size() << std::endl;
	
	log_index_t pre_index = 0;
	for(size_t i = 0; i < entries.size(); i++)
	{
		if (!pre_index)
		{
			pre_index = entries[i].index();
		}
		else
		{
			acl_assert(pre_index == entries[i].index() - 1);
		}
		/*std::cout << entries[i].log_data().substr(1000) << std::endl;*/

		pre_index = entries[i].index();
	}
}
void discard()
{
	std::map<log_index_t, log_index_t> log_infos = 
		log_manager_->logs_info();

	std::map<log_index_t, log_index_t>::iterator it = 
		log_infos.begin();

	int count = 0;
	for (; it != log_infos.end(); ++it)
	{
		if (count < log_infos.size() / 2)
		{
			count += log_manager_->discard_log(it->first);
		}
	}
	std::cout << "discard log count:" << count << std::endl;
}
int main()
{
	acl::log::stdout_open(true);
	//create
	create_log_manger();

	// reload log
    if (!log_manager_->reload_logs())
    {
        logger_error("reload log error");
        return 1;
    }

	//write
	int start = log_manager_->last_index() + 1;
	int end = start + 1000000;
	write(start, end);


	//log count
	std::cout << log_manager_->log_count() << std::endl;;

	//logs_info
	std::map<log_index_t, log_index_t> log_infos = 
		log_manager_->logs_info();

	std::map<log_index_t, log_index_t>::iterator it = 
		log_infos.begin();

	for(; it!= log_infos.end(); ++it)
	{
		std::cout << 
			"log_start: "<< 
			it->first << 
			"    last_log: "<< 
			it->second << 
			std::endl;
	}

	//read
	/*read(log_manager_->start_index(), log_manager_->last_index() + 1)*/;

	read_all();

	//discard log
	discard();

	//close_log_manager
	close_log_manager();
}