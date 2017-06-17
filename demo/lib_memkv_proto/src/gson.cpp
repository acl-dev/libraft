#include "acl_cpp/lib_acl.hpp"
#include "raft_config.h"
#include "cluster_config.h"
#include "memkv_proto.h"
#include "gson.h"
#include "acl_cpp/serialize/gson_helper.ipp"
namespace acl
{
    acl::json_node& gson(acl::json &$json, const addr_info &$obj)
    {
        acl::json_node &$node = $json.create_node();

        if (check_nullptr($obj.addr))
            $node.add_null("addr");
        else
            $node.add_text("addr", acl::get_value($obj.addr));

        if (check_nullptr($obj.id))
            $node.add_null("id");
        else
            $node.add_text("id", acl::get_value($obj.id));


        return $node;
    }
    
    acl::json_node& gson(acl::json &$json, const addr_info *$obj)
    {
        return gson ($json, *$obj);
    }


    acl::string gson(const addr_info &$obj)
    {
        acl::json $json;
        acl::json_node &$node = acl::gson ($json, $obj);
        return $node.to_string ();
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, addr_info &$obj)
    {
        acl::json_node *addr = $node["addr"];
        acl::json_node *id = $node["id"];
        std::pair<bool, std::string> $result;

        if(!addr ||!($result = gson(*addr, &$obj.addr), $result.first))
            return std::make_pair(false, "required [addr_info.addr] failed:{"+$result.second+"}");
     
        if(!id ||!($result = gson(*id, &$obj.id), $result.first))
            return std::make_pair(false, "required [addr_info.id] failed:{"+$result.second+"}");
     
        return std::make_pair(true,"");
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, addr_info *$obj)
    {
        return gson($node, *$obj);
    }


     std::pair<bool,std::string> gson(const acl::string &$str, addr_info &$obj)
    {
        acl::json _json;
        _json.update($str.c_str());
        if (!_json.finish())
        {
            return std::make_pair(false, "json not finish error");
        }
        return gson(_json.get_root(), $obj);
    }


    acl::json_node& gson(acl::json &$json, const cluster_config &$obj)
    {
        acl::json_node &$node = $json.create_node();

        if (check_nullptr($obj.addrs))
            $node.add_null("addrs");
        else
            $node.add_child("addrs", acl::gson($json, $obj.addrs));


        return $node;
    }
    
    acl::json_node& gson(acl::json &$json, const cluster_config *$obj)
    {
        return gson ($json, *$obj);
    }


    acl::string gson(const cluster_config &$obj)
    {
        acl::json $json;
        acl::json_node &$node = acl::gson ($json, $obj);
        return $node.to_string ();
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, cluster_config &$obj)
    {
        acl::json_node *addrs = $node["addrs"];
        std::pair<bool, std::string> $result;

        if(!addrs ||!addrs->get_obj()||!($result = gson(*addrs->get_obj(), &$obj.addrs), $result.first))
            return std::make_pair(false, "required [cluster_config.addrs] failed:{"+$result.second+"}");
     
        return std::make_pair(true,"");
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, cluster_config *$obj)
    {
        return gson($node, *$obj);
    }


     std::pair<bool,std::string> gson(const acl::string &$str, cluster_config &$obj)
    {
        acl::json _json;
        _json.update($str.c_str());
        if (!_json.finish())
        {
            return std::make_pair(false, "json not finish error");
        }
        return gson(_json.get_root(), $obj);
    }


    acl::json_node& gson(acl::json &$json, const del_req &$obj)
    {
        acl::json_node &$node = $json.create_node();

        if (check_nullptr($obj.key))
            $node.add_null("key");
        else
            $node.add_text("key", acl::get_value($obj.key));


        return $node;
    }
    
    acl::json_node& gson(acl::json &$json, const del_req *$obj)
    {
        return gson ($json, *$obj);
    }


    acl::string gson(const del_req &$obj)
    {
        acl::json $json;
        acl::json_node &$node = acl::gson ($json, $obj);
        return $node.to_string ();
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, del_req &$obj)
    {
        acl::json_node *key = $node["key"];
        std::pair<bool, std::string> $result;

        if(!key ||!($result = gson(*key, &$obj.key), $result.first))
            return std::make_pair(false, "required [del_req.key] failed:{"+$result.second+"}");
     
        return std::make_pair(true,"");
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, del_req *$obj)
    {
        return gson($node, *$obj);
    }


     std::pair<bool,std::string> gson(const acl::string &$str, del_req &$obj)
    {
        acl::json _json;
        _json.update($str.c_str());
        if (!_json.finish())
        {
            return std::make_pair(false, "json not finish error");
        }
        return gson(_json.get_root(), $obj);
    }


    acl::json_node& gson(acl::json &$json, const del_resp &$obj)
    {
        acl::json_node &$node = $json.create_node();

        if (check_nullptr($obj.status))
            $node.add_null("status");
        else
            $node.add_text("status", acl::get_value($obj.status));


        return $node;
    }
    
    acl::json_node& gson(acl::json &$json, const del_resp *$obj)
    {
        return gson ($json, *$obj);
    }


    acl::string gson(const del_resp &$obj)
    {
        acl::json $json;
        acl::json_node &$node = acl::gson ($json, $obj);
        return $node.to_string ();
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, del_resp &$obj)
    {
        acl::json_node *status = $node["status"];
        std::pair<bool, std::string> $result;

        if(!status ||!($result = gson(*status, &$obj.status), $result.first))
            return std::make_pair(false, "required [del_resp.status] failed:{"+$result.second+"}");
     
        return std::make_pair(true,"");
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, del_resp *$obj)
    {
        return gson($node, *$obj);
    }


     std::pair<bool,std::string> gson(const acl::string &$str, del_resp &$obj)
    {
        acl::json _json;
        _json.update($str.c_str());
        if (!_json.finish())
        {
            return std::make_pair(false, "json not finish error");
        }
        return gson(_json.get_root(), $obj);
    }


    acl::json_node& gson(acl::json &$json, const exist_req &$obj)
    {
        acl::json_node &$node = $json.create_node();

        if (check_nullptr($obj.key))
            $node.add_null("key");
        else
            $node.add_text("key", acl::get_value($obj.key));


        return $node;
    }
    
    acl::json_node& gson(acl::json &$json, const exist_req *$obj)
    {
        return gson ($json, *$obj);
    }


    acl::string gson(const exist_req &$obj)
    {
        acl::json $json;
        acl::json_node &$node = acl::gson ($json, $obj);
        return $node.to_string ();
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, exist_req &$obj)
    {
        acl::json_node *key = $node["key"];
        std::pair<bool, std::string> $result;

        if(!key ||!($result = gson(*key, &$obj.key), $result.first))
            return std::make_pair(false, "required [exist_req.key] failed:{"+$result.second+"}");
     
        return std::make_pair(true,"");
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, exist_req *$obj)
    {
        return gson($node, *$obj);
    }


     std::pair<bool,std::string> gson(const acl::string &$str, exist_req &$obj)
    {
        acl::json _json;
        _json.update($str.c_str());
        if (!_json.finish())
        {
            return std::make_pair(false, "json not finish error");
        }
        return gson(_json.get_root(), $obj);
    }


    acl::json_node& gson(acl::json &$json, const exist_resp &$obj)
    {
        acl::json_node &$node = $json.create_node();

        if (check_nullptr($obj.status))
            $node.add_null("status");
        else
            $node.add_text("status", acl::get_value($obj.status));


        return $node;
    }
    
    acl::json_node& gson(acl::json &$json, const exist_resp *$obj)
    {
        return gson ($json, *$obj);
    }


    acl::string gson(const exist_resp &$obj)
    {
        acl::json $json;
        acl::json_node &$node = acl::gson ($json, $obj);
        return $node.to_string ();
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, exist_resp &$obj)
    {
        acl::json_node *status = $node["status"];
        std::pair<bool, std::string> $result;

        if(!status ||!($result = gson(*status, &$obj.status), $result.first))
            return std::make_pair(false, "required [exist_resp.status] failed:{"+$result.second+"}");
     
        return std::make_pair(true,"");
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, exist_resp *$obj)
    {
        return gson($node, *$obj);
    }


     std::pair<bool,std::string> gson(const acl::string &$str, exist_resp &$obj)
    {
        acl::json _json;
        _json.update($str.c_str());
        if (!_json.finish())
        {
            return std::make_pair(false, "json not finish error");
        }
        return gson(_json.get_root(), $obj);
    }


    acl::json_node& gson(acl::json &$json, const get_req &$obj)
    {
        acl::json_node &$node = $json.create_node();

        if (check_nullptr($obj.key))
            $node.add_null("key");
        else
            $node.add_text("key", acl::get_value($obj.key));


        return $node;
    }
    
    acl::json_node& gson(acl::json &$json, const get_req *$obj)
    {
        return gson ($json, *$obj);
    }


    acl::string gson(const get_req &$obj)
    {
        acl::json $json;
        acl::json_node &$node = acl::gson ($json, $obj);
        return $node.to_string ();
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, get_req &$obj)
    {
        acl::json_node *key = $node["key"];
        std::pair<bool, std::string> $result;

        if(!key ||!($result = gson(*key, &$obj.key), $result.first))
            return std::make_pair(false, "required [get_req.key] failed:{"+$result.second+"}");
     
        return std::make_pair(true,"");
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, get_req *$obj)
    {
        return gson($node, *$obj);
    }


     std::pair<bool,std::string> gson(const acl::string &$str, get_req &$obj)
    {
        acl::json _json;
        _json.update($str.c_str());
        if (!_json.finish())
        {
            return std::make_pair(false, "json not finish error");
        }
        return gson(_json.get_root(), $obj);
    }


    acl::json_node& gson(acl::json &$json, const get_resp &$obj)
    {
        acl::json_node &$node = $json.create_node();

        if (check_nullptr($obj.status))
            $node.add_null("status");
        else
            $node.add_text("status", acl::get_value($obj.status));

        if (check_nullptr($obj.value))
            $node.add_null("value");
        else
            $node.add_text("value", acl::get_value($obj.value));


        return $node;
    }
    
    acl::json_node& gson(acl::json &$json, const get_resp *$obj)
    {
        return gson ($json, *$obj);
    }


    acl::string gson(const get_resp &$obj)
    {
        acl::json $json;
        acl::json_node &$node = acl::gson ($json, $obj);
        return $node.to_string ();
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, get_resp &$obj)
    {
        acl::json_node *status = $node["status"];
        acl::json_node *value = $node["value"];
        std::pair<bool, std::string> $result;

        if(!status ||!($result = gson(*status, &$obj.status), $result.first))
            return std::make_pair(false, "required [get_resp.status] failed:{"+$result.second+"}");
     
        if(!value ||!($result = gson(*value, &$obj.value), $result.first))
            return std::make_pair(false, "required [get_resp.value] failed:{"+$result.second+"}");
     
        return std::make_pair(true,"");
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, get_resp *$obj)
    {
        return gson($node, *$obj);
    }


     std::pair<bool,std::string> gson(const acl::string &$str, get_resp &$obj)
    {
        acl::json _json;
        _json.update($str.c_str());
        if (!_json.finish())
        {
            return std::make_pair(false, "json not finish error");
        }
        return gson(_json.get_root(), $obj);
    }


    acl::json_node& gson(acl::json &$json, const raft_config &$obj)
    {
        acl::json_node &$node = $json.create_node();

        if (check_nullptr($obj.log_path))
            $node.add_null("log_path");
        else
            $node.add_text("log_path", acl::get_value($obj.log_path));

        if (check_nullptr($obj.snapshot_path))
            $node.add_null("snapshot_path");
        else
            $node.add_text("snapshot_path", acl::get_value($obj.snapshot_path));

        if (check_nullptr($obj.metadata_path))
            $node.add_null("metadata_path");
        else
            $node.add_text("metadata_path", acl::get_value($obj.metadata_path));

        if (check_nullptr($obj.max_log_size))
            $node.add_null("max_log_size");
        else
            $node.add_number("max_log_size", acl::get_value($obj.max_log_size));

        if (check_nullptr($obj.max_log_count))
            $node.add_null("max_log_count");
        else
            $node.add_number("max_log_count", acl::get_value($obj.max_log_count));

        if (check_nullptr($obj.peer_addrs))
            $node.add_null("peer_addrs");
        else
            $node.add_child("peer_addrs", acl::gson($json, $obj.peer_addrs));

        if (check_nullptr($obj.node_addr))
            $node.add_null("node_addr");
        else
            $node.add_child("node_addr", acl::gson($json, $obj.node_addr));


        return $node;
    }
    
    acl::json_node& gson(acl::json &$json, const raft_config *$obj)
    {
        return gson ($json, *$obj);
    }


    acl::string gson(const raft_config &$obj)
    {
        acl::json $json;
        acl::json_node &$node = acl::gson ($json, $obj);
        return $node.to_string ();
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, raft_config &$obj)
    {
        acl::json_node *log_path = $node["log_path"];
        acl::json_node *snapshot_path = $node["snapshot_path"];
        acl::json_node *metadata_path = $node["metadata_path"];
        acl::json_node *max_log_size = $node["max_log_size"];
        acl::json_node *max_log_count = $node["max_log_count"];
        acl::json_node *peer_addrs = $node["peer_addrs"];
        acl::json_node *node_addr = $node["node_addr"];
        std::pair<bool, std::string> $result;

        if(!log_path ||!($result = gson(*log_path, &$obj.log_path), $result.first))
            return std::make_pair(false, "required [raft_config.log_path] failed:{"+$result.second+"}");
     
        if(!snapshot_path ||!($result = gson(*snapshot_path, &$obj.snapshot_path), $result.first))
            return std::make_pair(false, "required [raft_config.snapshot_path] failed:{"+$result.second+"}");
     
        if(!metadata_path ||!($result = gson(*metadata_path, &$obj.metadata_path), $result.first))
            return std::make_pair(false, "required [raft_config.metadata_path] failed:{"+$result.second+"}");
     
        if(!max_log_size ||!($result = gson(*max_log_size, &$obj.max_log_size), $result.first))
            return std::make_pair(false, "required [raft_config.max_log_size] failed:{"+$result.second+"}");
     
        if(!max_log_count ||!($result = gson(*max_log_count, &$obj.max_log_count), $result.first))
            return std::make_pair(false, "required [raft_config.max_log_count] failed:{"+$result.second+"}");
     
        if(!peer_addrs ||!peer_addrs->get_obj()||!($result = gson(*peer_addrs->get_obj(), &$obj.peer_addrs), $result.first))
            return std::make_pair(false, "required [raft_config.peer_addrs] failed:{"+$result.second+"}");
     
        if(!node_addr ||!node_addr->get_obj()||!($result = gson(*node_addr->get_obj(), &$obj.node_addr), $result.first))
            return std::make_pair(false, "required [raft_config.node_addr] failed:{"+$result.second+"}");
     
        return std::make_pair(true,"");
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, raft_config *$obj)
    {
        return gson($node, *$obj);
    }


     std::pair<bool,std::string> gson(const acl::string &$str, raft_config &$obj)
    {
        acl::json _json;
        _json.update($str.c_str());
        if (!_json.finish())
        {
            return std::make_pair(false, "json not finish error");
        }
        return gson(_json.get_root(), $obj);
    }


    acl::json_node& gson(acl::json &$json, const set_req &$obj)
    {
        acl::json_node &$node = $json.create_node();

        if (check_nullptr($obj.key))
            $node.add_null("key");
        else
            $node.add_text("key", acl::get_value($obj.key));

        if (check_nullptr($obj.value))
            $node.add_null("value");
        else
            $node.add_text("value", acl::get_value($obj.value));


        return $node;
    }
    
    acl::json_node& gson(acl::json &$json, const set_req *$obj)
    {
        return gson ($json, *$obj);
    }


    acl::string gson(const set_req &$obj)
    {
        acl::json $json;
        acl::json_node &$node = acl::gson ($json, $obj);
        return $node.to_string ();
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, set_req &$obj)
    {
        acl::json_node *key = $node["key"];
        acl::json_node *value = $node["value"];
        std::pair<bool, std::string> $result;

        if(!key ||!($result = gson(*key, &$obj.key), $result.first))
            return std::make_pair(false, "required [set_req.key] failed:{"+$result.second+"}");
     
        if(!value ||!($result = gson(*value, &$obj.value), $result.first))
            return std::make_pair(false, "required [set_req.value] failed:{"+$result.second+"}");
     
        return std::make_pair(true,"");
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, set_req *$obj)
    {
        return gson($node, *$obj);
    }


     std::pair<bool,std::string> gson(const acl::string &$str, set_req &$obj)
    {
        acl::json _json;
        _json.update($str.c_str());
        if (!_json.finish())
        {
            return std::make_pair(false, "json not finish error");
        }
        return gson(_json.get_root(), $obj);
    }


    acl::json_node& gson(acl::json &$json, const set_resp &$obj)
    {
        acl::json_node &$node = $json.create_node();

        if (check_nullptr($obj.status))
            $node.add_null("status");
        else
            $node.add_text("status", acl::get_value($obj.status));


        return $node;
    }
    
    acl::json_node& gson(acl::json &$json, const set_resp *$obj)
    {
        return gson ($json, *$obj);
    }


    acl::string gson(const set_resp &$obj)
    {
        acl::json $json;
        acl::json_node &$node = acl::gson ($json, $obj);
        return $node.to_string ();
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, set_resp &$obj)
    {
        acl::json_node *status = $node["status"];
        std::pair<bool, std::string> $result;

        if(!status ||!($result = gson(*status, &$obj.status), $result.first))
            return std::make_pair(false, "required [set_resp.status] failed:{"+$result.second+"}");
     
        return std::make_pair(true,"");
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, set_resp *$obj)
    {
        return gson($node, *$obj);
    }


     std::pair<bool,std::string> gson(const acl::string &$str, set_resp &$obj)
    {
        acl::json _json;
        _json.update($str.c_str());
        if (!_json.finish())
        {
            return std::make_pair(false, "json not finish error");
        }
        return gson(_json.get_root(), $obj);
    }


}///end of acl.
