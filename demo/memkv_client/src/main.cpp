//
// Created by akzi on 17-6-17.
//

#include <iostream>
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
    client.set_cluster(config);
    client.print_service_info();
}

int main(int argc, char *argv[])
{
    acl::acl_cpp_init();

    acl::log::stdout_open(true);

    init_client();

    if(argc <2)
        logger_fatal("param error");

    const char *cmd = argv[1];

    std::cout << "cmd:" << cmd << std::endl;

    if(cmd == std::string("get"))
    {
        if(argc != 3)
            logger_fatal("Param Error.\n memkv_client get key");

        std::string key = argv[2];

        std::pair<bool, std::string> result = client.get(key);
        if(result.first)
        {
            std::cout << "get " << key << " ok . "<< result.second << std::endl;
        }else
            std::cout << "get " <<key << "failed. "<<result.second  << std::endl;
    }
    else if(cmd == std::string("set"))
    {
        std::string key = argv[2];
        std::string value  = argv[3];
        for(int i = 0 ; i < atoi(argv[4]); i++)
        {
            std::cout << client.set(key,value) << std::endl;
        }

    }else if(cmd == std::string("del"))
    {
        std::string key = argv[2];

        std::cout << client.del(key) << std::endl;

    }
    else if(cmd == std::string("exist"))
    {
        std::string key = argv[2];

        std::cout << client.exist(key).second << std::endl;
    }else
    {
        logger("unknown cmd:%s",cmd);
    }
}
