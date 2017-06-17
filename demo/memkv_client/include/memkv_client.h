//
// Created by akzi on 17-6-17.
//
#pragma once

class memkv_client
{
public:
    memkv_client();
    ~memkv_client();
    void set_cluster(std::vector<std::string> addrs);
    
};
