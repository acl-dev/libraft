#pragma once
#include "raft.hpp"
#include "raft_config.h"
#include <map>

class memkv_service :acl::service_base
{
public:
	explicit memkv_service(acl::http_rpc_server &server)
		:acl::service_base(server)
	{
	}

	virtual ~memkv_service()
	{

	}
private:
	virtual void init()
	{
		raft_config cfg = get_raft_config();
		node_ = new raft::node;
		node_->set_log_path(cfg.log_path);
		node_->set_max_log_size(cfg.max_log_size);
		node_->set_max_log_count(cfg.max_log_count);
		node_->set_metadata_path(cfg.metadata_path);
		node_->set_snapshot_path(cfg.snapshot_path);
	}
	static raft_config get_raft_config()
	{
		return{};
	}
	raft::node *node_;
};

class memkv :public acl::http_rpc_server
{
public:
	memkv()
	{

	}
	virtual ~memkv()
	{

	}
private:
	virtual void init()
	{

	}
};


