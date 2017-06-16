#pragma once
namespace acl
{
    //addr_info
    acl::string gson(const addr_info &$obj);
    acl::json_node& gson(acl::json &$json, const addr_info &$obj);
    acl::json_node& gson(acl::json &$json, const addr_info *$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, addr_info &$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, addr_info *$obj);
    std::pair<bool,std::string> gson(const acl::string &str, addr_info &$obj);

    //raft_config
    acl::string gson(const raft_config &$obj);
    acl::json_node& gson(acl::json &$json, const raft_config &$obj);
    acl::json_node& gson(acl::json &$json, const raft_config *$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, raft_config &$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, raft_config *$obj);
    std::pair<bool,std::string> gson(const acl::string &str, raft_config &$obj);

}///end of acl.
