//
// Created by akzi on 17-6-17.
//

#include <iostream>
#include "cluster_config.h"
#include "memkv_client.h"

memkv_client client;
void init_client()
{
    acl::ifstream file;
    acl::string buffer;
    cluster_config config;

    acl_assert(file.open_read("cluster_config.json"));
    file.load(&buffer);

    std::pair<bool, std::string> ret = acl::gson(buffer, config);
    if(!ret.first)
        logger_error("load cluster config error");
    client.set_cluster(config.addrs);
}

int main(int argc, char *argv[])
{
    init_client();
    const char *cmd = argv[1];

    if(cmd == std::string("get"))
    {
        std::string key = argv[2];

        std::cout << client.get(key).second << std::endl;
    }
    else if(cmd == std::string("set"))
    {
        std::string key = argv[2];
        std::string value  = argv[3];

        std::cout << client.set(key,value) << std::endl;

    }else if(cmd == std::string("del"))
    {
        std::string key = argv[2];

        std::cout << client.del(key) << std::endl;

    }
    else if(cmd == std::string("exist"))
    {
        std::string key = argv[2];

        std::cout << client.exist(key).second << std::endl;
    }
}