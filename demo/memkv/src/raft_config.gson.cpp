#include "acl_cpp/lib_acl.hpp"
#include "raft_config.h"
#include "raft_config.gson.h"
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


}///end of acl.
