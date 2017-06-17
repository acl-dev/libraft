//
// Created by akzi on 17-6-17.
//
#pragma once
#include <string>
#include <utility>
#include <vector>
#include "acl_cpp/lib_acl.hpp"
#include "memkv_proto.h"
#include "raft_config.h"
#include "gson.h"
#include "http_rpc.h"


class memkv_client {
public:
    typedef acl::http_rpc_client::status_t status_t;

    memkv_client()
        :rpc_client(acl::http_rpc_client::get_instance())
    {

    }

    ~memkv_client()
    {

    }

    void set_cluster(const std::vector<std::string> &addrs) {
        std::vector<std::string> services;
        services.push_back("memkv/get");
        services.push_back("memkv/set");
        services.push_back("memkv/del");
        services.push_back("memkv/exist");

        for (size_t j = 0; j < services.size(); ++j)
        {
            for (size_t i = 0; i < addrs.size(); ++i)
            {

                acl::string service_path;
                std::string action;
                service_path.format("%s/%s",
                                    addrs[i].c_str(),
                                    services[j].c_str());
                action = service_path.c_str();
                action = action.substr(0, action.find_last_of('/'));

                rpc_client.add_service(addrs[i].c_str(),service_path,30,30);

                services_[action].push_back(addrs[i]);
            }
        }
    }

    std::pair<bool, std::string> get(const std::string &key)
    {


        get_req req;
        get_resp resp;
        std::vector<std::string> services = services_["get"];

        req.key = key;

        for (int i = 0; i < services.size(); ++i) {

            status_t status =
                    rpc_client.json_call(
                            services[i].c_str(),
                            req,
                            resp);
            if (status) {
                if (resp.status == "ok")
                    return std::make_pair(true, resp.value);
                logger("get response error. %s", resp.status.c_str());
            }
        }
        return std::make_pair(false, "");
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

private:
    acl::http_rpc_client &rpc_client;
    std::map<std::string, std::vector<std::string> > services_;
};
