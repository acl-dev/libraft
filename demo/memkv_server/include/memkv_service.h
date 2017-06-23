#pragma once

struct memkv_load_snapshot_callback;
struct memkv_make_snapshot_callback;
struct memkv_apply_callback;

class memkv_service : public acl::service_base
{
public:
	explicit memkv_service(acl::http_rpc_server &server);

	~memkv_service();
private:
	friend struct memkv_load_snapshot_callback;
	friend struct memkv_make_snapshot_callback;
	friend struct memkv_apply_callback;

	typedef std::map<std::string, std::string> memkv_store_t;
	

	virtual void init();

	void load_config();

	void init_raft_node();

	void regist_service();

    void reload();
	//raft from raft framework
	bool load_snapshot(const std::string &file_path);

	bool make_snapshot(const std::string &path, std::string &file_path);

	bool apply(const std::string& data, const raft::version& ver);
	//end

	//helper function
	bool check_leader()const;
	
	//memkv services
	bool get(const get_req &req, get_resp &resp);

	bool exist(const exist_req &req, exist_resp& resp);

	bool set(const set_req &req, set_resp &resp);

	bool del(const del_req &req, del_resp &resp);

    void do_print_status();
private:
    struct print_status :public  acl::thread
    {
        void *run()
        {
            is_stop_ = false;
            do
            {
                memkv_service_->do_print_status();
                acl_doze(1000);
            }while(!is_stop_);
        }
        bool is_stop_;
        memkv_service *memkv_service_;
    };

    //raft callback handles
    memkv_load_snapshot_callback *load_snapshot_callback_;
    memkv_make_snapshot_callback *make_snapshot_callback_;
    memkv_apply_callback         *apply_callback_;

	//raft node
	raft::node *node_;

	// config 
	raft_config cfg_;

    //kv store
	memkv_store_t   store_;
    raft::version   curr_ver_;
    acl::locker     mem_store_locker_;

	//config file_path
	std::string cfg_file_path_;

    size_t writes_;
    size_t last_writes_;

    print_status *print_status_;
};
