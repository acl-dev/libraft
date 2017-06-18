#pragma once
#include <string>
#include <vector>

struct raft_config
{
	std::string log_path;
	std::string snapshot_path;
	std::string metadata_path;
	
	int max_log_size;
	//max log count of raft log
	int max_log_count;
	//cluster addrs without myself
	std::vector<addr_info> peer_addrs;
	//myself addr
	addr_info node_addr;
};
