#pragma once


class memkv_service :acl::service_base
{
public:
	explicit memkv_service(acl::http_rpc_server &server);

	~memkv_service();
private:
	friend class memkv_load_snapshot_callback;
	friend class memkv_make_snapshot_callback;
	friend class memkv_apply_callback;

	typedef std::map<std::string, std::string> memkv_store_t;
	

	virtual void init();

	void load_config();

	void init_http_rpc_client();

	void init_raft_node();

	void regist_service();

	//raft from raft framework
	bool load_snapshot(const std::string &filepath);

	bool make_snapshot(const std::string &path, std::string &filepath);

	bool apply(const std::string& data, const raft::version& ver);
	//end

	//helper function
	bool check_leader()const;
	//memkv serivces

	bool get(const get_req &req, get_resp &resp);

	bool exist(const exist_req &req, exist_resp& resp);

	bool set(const set_req &req, set_resp &resp);

	bool del(const del_req &req, del_resp &resp);

	//raft callback handles
	memkv_load_snapshot_callback *load_snapshot_callback_;
	memkv_make_snapshot_callback *make_snapshot_callback_;
	memkv_apply_callback *apply_callback_;

	//raft node
	raft::node *node_;

	// config 
	raft_config cfg_;

	//kv store
	memkv_store_t store_;
	raft::version curr_ver_;
	acl::locker mem_store_locker_;
};
