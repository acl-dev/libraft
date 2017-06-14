#pragma once
#include <string>
#include <vector>

struct peer_info
{
	std::string addr;
	std::string id;
};
struct raft_config
{
	std::string log_path;
	std::string snapshot_path;
	std::string metadata_path;
	int max_log_size;
	int max_log_count;
	std::vector<peer_info> peer_infos;
};