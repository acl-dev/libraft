//
// Created by akzi on 17-6-17.
//
#pragma once
#include <string>
#include <utility>
#include <vector>
#include "acl_cpp/lib_acl.hpp"
#include "memkv_proto.h"
#include "addr_info.h"
#include "cluster_config.h"
#include "raft_config.h"
#include "gson.h"
#include "http_rpc.h"


class memkv_client {
public:
    typedef acl::http_rpc_client::status_t status_t;
    typedef std::map<std::string, std::vector<std::string> > service_map_t;
    typedef service_map_t::iterator service_map_iterator_t;

    memkv_client()
        :rpc_client(acl::http_rpc_client::get_instance())
    {

    }

    ~memkv_client()
    {

    }

    void set_cluster(const cluster_config &_cluster_config) {
        std::vector<std::string> services;
        services.push_back("store/get");
        services.push_back("store/set");
        services.push_back("store/del");
        services.push_back("store/exist");

        for (size_t j = 0; j < services.size(); ++j)
        {
            for (size_t i = 0; i < _cluster_config.addrs.size(); ++i)
            {

                acl::string service_path;
                std::string action;
                std::string addr = _cluster_config.addrs[i].addr;
                std::string id = _cluster_config.addrs[i].id;

                service_path.format("/memkv%s/%s",id.c_str(),
                                    services[j].c_str());

                action = service_path.c_str();

                action = action.substr(action.find_last_of('/') + 1);

                rpc_client.add_service(addr.c_str(), service_path);

                logger("addr:%s service_path:%s",
                       addr.c_str(),
                       service_path.c_str());

                services_[action].push_back(service_path.c_str());
            }
        }
    }

    std::pair<bool, std::string> get(const std::string &key)
    {


        get_req req;
        get_resp resp;
        std::vector<std::string> services = services_["get"];

        if(services.empty())
            logger_fatal("not service to call");

        req.key = key;

        for (int i = 0; i < services.size(); ++i)
        {

            std::string service_path = services[i].c_str();
            logger("do json call. service_path:%s", service_path.c_str());
            status_t status =
                    rpc_client.json_call(
                            service_path.c_str(),
                            req,
                            resp);

            if (status) {
                if (resp.status == "ok")
                    return std::make_pair(true, resp.value);
                logger("get response error. %s", resp.status.c_str());
            }
            logger("json_call error. %s", status.error_str_.c_str());

        }
        return std::make_pair(false, "get failed");
    };

    std::string set(const std::string &key, const std::string &value)
    {
        set_req req;
        set_resp resp;
        std::vector<std::string> services = services_["set"];

        req.key = key;
        req.value = value;

        for (int i = 0; i < services.size(); ++i)
        {
            status_t status =
                    rpc_client.json_call(
                            services[i].c_str(),req, resp);

            if (status) {
                if (resp.status == "ok")
                    return resp.status;

                logger("set response error. %s",
                       resp.status.c_str());
            }
        }
        return "del request failed";
    }

    std::string del(const std::string &key)
    {
        del_req req;
        del_resp resp;
        std::vector<std::string> services = services_["det"];

        req.key = key;

        for (int i = 0; i < services.size(); ++i)
        {

            status_t status =
                    rpc_client.json_call(
                            services[i].c_str(), req, resp);
            if (status) {
                if (resp.status == "ok")
                    return resp.status;

                logger("del response error. %s",
                       resp.status.c_str());
            }
        }
        return "del request failed";
    }

    std::pair<bool, std::string> exist(const std::string &key)
    {
        exist_req req;
        exist_resp resp;
        std::vector<std::string> services = services_["det"];

        req.key = key;

        for (int i = 0; i < services.size(); ++i)
        {

            status_t status =
                    rpc_client.json_call(
                            services[i].c_str(), req, resp);
            if (status) {
                if (resp.status != "no leader")
                    return std::make_pair(true, resp.status);

                logger("exist response error. %s",
                       resp.status.c_str());
            }
        }
        return std::make_pair(false,std::string("exist request failed"));
    };
    void print_service_info()
    {
        logger("-------------------services----------------------");
        service_map_iterator_t it = services_.begin();
        for (; it != services_.end(); ++it)
        {
            logger("%s services:",it->first.c_str());
            for (size_t i = 0; i < it->second.size(); ++i)
            {
                logger("           %s",it->second[i].c_str());
            }
        }
        logger("-------------------services----------------------");
    }
private:
    acl::http_rpc_client &rpc_client;
    service_map_t services_;
};
