#include "acl_cpp/lib_acl.hpp"
#include "lib_acl.h"
#include "addr_info.h"
#include "raft_config.h"
#include "memkv_proto.h"
#include "cluster_config.h"
#include "gson.h"
#include "raft.hpp"
#include "memkv_service.h"

extern char *var_cfg_raft_config;

typedef raft::replicate_callback::status_t replicate_status_t;

struct memkv_load_snapshot_callback :raft::load_snapshot_callback
{
	explicit memkv_load_snapshot_callback(memkv_service *memkv)
		:memkv_service_(memkv)
	{

	}
	virtual bool operator()(const std::string &file_path)
	{
		return memkv_service_->load_snapshot(file_path);
	}
	memkv_service *memkv_service_;
};
struct memkv_make_snapshot_callback :raft::make_snapshot_callback
{
	explicit memkv_make_snapshot_callback(memkv_service *memkv)
		:memkv_service_(memkv)
	{

	}
	virtual bool operator()(const std::string &path, std::string &file_path)
	{
		return memkv_service_->make_snapshot(path, file_path);
	}
	memkv_service *memkv_service_;
};
struct memkv_apply_callback : raft::apply_callback
{
	explicit memkv_apply_callback(memkv_service *memkv)
		:memkv_service_(memkv)
	{

	}
	virtual bool operator()(const std::string& data, const raft::version& ver)
	{
		return memkv_service_->apply(data, ver);
	}
	memkv_service *memkv_service_;
};

class replicate_future : public raft::replicate_callback
{
public:
	replicate_future()
		:done_(false)
	{
		acl_assert(acl_pthread_cond_init(&cond_, NULL) == 0);
		acl_assert(acl_pthread_mutex_init(&mutex_, NULL) == 0);
	}

	~replicate_future()
	{
		acl_pthread_cond_destroy(&cond_);
		acl_pthread_mutex_destroy(&mutex_);
	}
	bool operator()(status_t status, raft::version ver)
	{
		done_ = true;
		status_ = status;
		ver_ = ver;
		//notify .replicate done or error
		acl_pthread_cond_signal(&cond_);
		return true;
	}
	//wait for replicate dong or error
	void wait()
	{
		acl_pthread_mutex_lock(&mutex_);
		//if done. not wait any more
		if (done_)
		{
			acl_pthread_mutex_unlock(&mutex_);
			return;
		}
		acl_pthread_cond_wait(&cond_, &mutex_);
		acl_pthread_mutex_unlock(&mutex_);
	}
	raft::version version()
	{
		return ver_;
	}
	replicate_status_t status()
	{
		return status_;
	}
private:
	acl_pthread_mutex_t mutex_;
	acl_pthread_cond_t cond_;
	status_t status_;
	raft::version ver_;
	bool done_;
};
static inline std::string to_string(const acl::string &data)
{
	return std::string(data.c_str(), data.size());
}

#define	DEL_REQ  'd'
#define	SET_REQ  's'

char get_req_flag(...)
{
	logger_error("not error req type");
	return '.';
}
char get_req_flag(const set_req &)
{
	return SET_REQ;
}
char get_req_flag(const del_req &)
{
	return DEL_REQ;
}
//do replicate req. and set RESP::status
//return true if replicate ok.otherwise return false
template<class REQ, class RESP>
bool replicate(const REQ& req,
	RESP &resp,
	raft::node *node,
	raft::version &version)
{
	std::string data = to_string(acl::gson(req));
	data.push_back(get_req_flag(req));
	replicate_future future;
	if (!node->replicate(data, &future))
	{
		resp.status = "error";
		return false;
	}
	future.wait();
	//maybe node lost leadership.
	replicate_status_t status = future.status();
	if (status == raft::replicate_callback::E_NO_LEADER)
	{
		resp.status = "no leader";
		return false;
	}
	else if (status == raft::replicate_callback::E_ERROR)
	{
		resp.status = "error";
		return false;
	}
	resp.status = "ok";
	version = future.version();
	return true;
}

memkv_service::memkv_service(acl::http_rpc_server &server)
	:acl::service_base(server)
{
	acl_assert(var_cfg_raft_config);

	load_snapshot_callback_ = new memkv_load_snapshot_callback(this);
	make_snapshot_callback_ = new memkv_make_snapshot_callback(this);
	apply_callback_ = new memkv_apply_callback(this);
	node_ = new raft::node;
	cfg_file_path_ = var_cfg_raft_config;
}

memkv_service::~memkv_service()
{
	delete node_;
	delete load_snapshot_callback_;
	delete make_snapshot_callback_;
	delete apply_callback_;
}

void memkv_service::init()
{
	load_config();
	init_http_rpc_client();
	init_raft_node();
	regist_service();

    reload();
    //start node
    node_->start();
}
void memkv_service::load_config()
{
	acl::ifstream file;
	if (!file.open_read(cfg_file_path_.c_str()))
	{
		logger_fatal("create_new_file config file error."
			"cfg_file_path_:%s "
			"error:%s",
			cfg_file_path_.c_str(),
			acl::last_serror());
		return;
	}

	acl::string data;
	acl_assert(file.load(&data));

	std::pair<bool, std::string> ret =
			acl::gson(data.c_str(), cfg_);

	if (!ret.first)
	{
		logger_fatal("gson error.%s", ret.second.c_str());
	}
}
void memkv_service::init_http_rpc_client()
{
	acl::http_rpc_client &rpc_client = 
		acl::http_rpc_client::get_instance();

	std::vector<std::string> paths;

    paths.push_back("raft/vote_req");
	paths.push_back("raft/replicate_log_req");
	paths.push_back("raft/install_snapshot_req");


	for (size_t i = 0; i < cfg_.peer_addrs.size(); i++)
	{
		for (size_t j = 0; j < paths.size(); j++)
		{
			acl::string service_path;
			const char *addr = cfg_.peer_addrs[i].addr.c_str();
			const char *id = cfg_.peer_addrs[i].id.c_str();

			service_path.format("/memkv%s/%s", id, paths[j].c_str());

			rpc_client.add_service(addr, service_path);

			logger("add service:"
						   "id:%s"
						   "addr:%s "
						   "service_path:%s",
						   id,
						   addr,
						   service_path.c_str());
		}
	}
}
void memkv_service::init_raft_node()
{
	node_ = new raft::node;
	node_->set_log_path(cfg_.log_path);
	node_->set_max_log_size((size_t) cfg_.max_log_size);
	node_->set_max_log_count((size_t) cfg_.max_log_count);
	node_->set_metadata_path(cfg_.metadata_path);
	node_->set_snapshot_path(cfg_.snapshot_path);

	std::vector<std::string> peers;
	for (size_t i = 0; i < cfg_.peer_addrs.size(); i++)
	{
		peers.push_back(cfg_.peer_addrs[i].id);
	}
	node_->set_peers(peers);

	//load snapshot 
	node_->set_load_snapshot_callback(load_snapshot_callback_);
	//make snapshot 
	node_->set_make_snapshot_callback(make_snapshot_callback_);
	//apply callback
	node_->set_apply_callback(apply_callback_);
}
void memkv_service::regist_service()
{
	//server id
	const char *id = cfg_.node_addr.id.c_str();
	acl::string service_path;

	//regist service for user client

	service_path.format("/memkv%s/store/get",id);
	server_.on_json(service_path, this, &memkv_service::get);

	service_path.format("/memkv%s/store/set",id);
	server_.on_json(service_path, this, &memkv_service::set);

	service_path.format("/memkv%s/store/del",id);
	server_.on_json(service_path, this, &memkv_service::del);

	service_path.format("/memkv%s/store/exist",id);
	server_.on_json(service_path, this, &memkv_service::exist);

	//regist service for raft peer

    //election req
    service_path.format("/memkv%s/raft/vote_req", id);
    server_.on_pb(service_path, node_,
                  &raft::node::handle_vote_request);

	//replicate req
	service_path.format("/memkv%s/raft/replicate_log_req", id);
	server_.on_pb(service_path, node_,
		&raft::node::handle_replicate_log_request);

	//snapshot req
	service_path.format("/memkv%s/raft/install_snapshot_req", id);
	server_.on_pb(service_path, node_,
                  &raft::node::handle_install_snapshot_request);

}
void memkv_service::reload()
{
    std::string file_path = node_->get_snapshot();
    if(file_path.size())
    {
        if(!load_snapshot(file_path))
            logger_fatal("load_snapshot error");
    }
    raft::log_index_t applied_index = node_->applied_index();
    acl_assert(curr_ver_.index_ <= applied_index);
    do
    {
        if(curr_ver_.index_ == applied_index)
        {
            logger("------reload ok! --------");
            logger("---kv_store size(%lu)----",store_.size());
            break;
        }


        std::string data;
        if(!node_->read(curr_ver_.index_+1, data, curr_ver_))
        {
            logger_fatal("read log failed!!!!!!!!!");
            return;
        }

        apply(data, curr_ver_);

    }while(true);
}
//raft from raft framework
bool memkv_service::load_snapshot(const std::string &file_path)
{
	acl::ifstream file;
	if (!file.open_read(file_path.c_str()))
	{
		logger_error("create_new_file file error");
		return false;
	}
	raft::version ver;
	if (!raft::read(file, ver))
	{
		logger_error("read version failed");
		return false;
	}
	if (ver < curr_ver_)
	{
		logger_fatal("rust version.");
		return false;
	}
	unsigned int items = 0;
	if (!raft::read(file, items))
	{
		logger_error("read snapshot items.error");
		return true;
	}
	acl::lock_guard lg(mem_store_locker_);

	//clear old data.because of newer snapshot.
	store_.clear();
	while (!file.eof())
	{
		std::string key;
		std::string value;
		if (raft::read(file, key) && raft::read(file, value))
		{
			store_.insert(std::make_pair(key, value));
		}
	}
	if (store_.size() != items)
	{
		logger_error("snapshot not finished");
		return false;
	}
	file.close();

	logger("load_snapshot %s done.items:%u",
		file_path.c_str(), items);

	return true;
}
bool memkv_service::make_snapshot(const std::string &path,
	std::string &file_path)
{
	acl::lock_guard lg(mem_store_locker_);
	
	acl::string snapshot_path;
	snapshot_path += path.c_str();
	/**
		file extension must not ".snapshot".
		when making snapshot,it maybe 
		happen something error.
		eg: power off, disk error, and so on.
		".snapshot" mean good snapshot file.
	*/
	snapshot_path.format_append("%llu.%llu.temp_snapshot",
				curr_ver_.index_, 
				curr_ver_.term_);

	acl::ofstream file;
	if (!file.open_trunc(snapshot_path.c_str()))
	{
		logger_error("create_new_file file error.%s",
                     snapshot_path.c_str());

		return false;
	}


	//write raft::version .and store item count
	if (!raft::write(file, curr_ver_) ||
		raft::write(file, (unsigned int)store_.size()))
	{
		goto failed;
	}

	for (memkv_store_t::const_iterator it = store_.begin();
		it != store_.end(); it++)
	{
		//write store key, and value
		if (!raft::write(file, it->first) ||
			!raft::write(file, it->second))
		{
			goto failed;
		}
	}

	file.close();
	file_path = snapshot_path;
	return true;
failed:
	logger_error("write snapshot file error!!!!!!!!!!!!!");
	file.close();
	remove(snapshot_path.c_str());
	return false;
}
/*
	apply invoke from raft framework.it mean leader replicate
	data to this node, and data has be committed.
*/
bool memkv_service::apply(const std::string& data,
	const raft::version& ver)
{
	acl::lock_guard lg(mem_store_locker_);
	if (data.empty())
		return false;

	char flag = data[data.size() - 1];
	std::pair<bool, std::string> status;
	if (flag == SET_REQ)
	{
		set_req req;
		status = acl::gson(data.c_str(), req);
		if (!status.first)
		{
			logger_error("req gson error");
			return false;
		}
		store_[req.key] = req.value;
        curr_ver_ = ver;
		return true;
	}
	else if (flag == DEL_REQ)
	{
		del_req req;
		status = acl::gson(data.c_str(), req);
		if (!status.first)
		{
			logger_error("req gson error");
			return false;
		}
		store_.erase(req.key);
        curr_ver_ = ver;
		return true;
	}
	logger_error("error req cmd");
	return false;
}
//end

//helper function
bool memkv_service::check_leader()const
{
	return node_->is_leader();
}

//memkv services
bool memkv_service::get(const get_req &req, get_resp &resp)
{
	if (!check_leader())
	{
		resp.status = "no leader";
		return true;
	}

	acl::lock_guard lg(mem_store_locker_);
	memkv_store_t::iterator it = store_.find(req.key);
	if (it != store_.end())
		resp.value = it->second;
	else
		resp.status = "not found";
	return true;
}

bool memkv_service::exist(const exist_req &req, exist_resp& resp)
{
	if (!check_leader())
	{
		resp.status = "no leader";
		return true;
	}
	acl::lock_guard lg(mem_store_locker_);
	memkv_store_t::iterator it = store_.find(req.key);
	if (it != store_.end())
		resp.status = "yes";
	else
		resp.status = "no";
	return true;
}


bool memkv_service::set(const set_req &req, set_resp &resp)
{
	if (!check_leader())
	{
		resp.status = "no leader";
		return true;
	}

	raft::version ver;
	if (!replicate(req, resp, node_, ver))
	{
		logger("set failed");
		return true;
	}
	// status ok .set key to store
	acl::lock_guard lg(mem_store_locker_);

	store_[req.key] = store_[req.value];
	curr_ver_ = ver;

	return true;
}
bool memkv_service::del(const del_req &req, del_resp &resp)
{

	if (!check_leader())
	{
		resp.status = "no leader";
		return true;
	}

	raft::version ver;
	if (!replicate(req, resp, node_, ver))
	{
		logger("set failed");
		return true;
	}

	// status ok .del value from store
	acl::lock_guard lg(mem_store_locker_);
	store_.erase(req.key);
	curr_ver_ = ver;

	return true;
}
