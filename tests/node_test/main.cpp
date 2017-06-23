//
// Created by akzi on 17-6-23.
//
#include "raft.hpp"

using namespace raft;
class node_test :public node
{
public:
    node_test()
    {

    }
    void do_test()
    {
        acl_assert(reload());
        do_build_replicate_log_request_test();
    }
    void do_build_replicate_log_request_test()
    {
        for (int i = 0; i < 10000000; ++i)
        {
            replicate_log_entries_request req;
            replicate_log_entries_response resp;
            acl::http_rpc_client::status_t status;

            log_index_t start = start_log_index();
            acl_assert(this->build_replicate_log_request(req, start+1, 1024));
        }

    }
};

int main()
{
    node_test().do_test();
    return 0;
}