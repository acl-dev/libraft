//
// Created by akzi on 17-6-23.
//

#include "raft.hpp"

using namespace raft;

void test_add_entry()
{
    replicate_log_entries_request req;

    req.set_term(1);
    req.set_leader_commit(1);
    req.set_leader_id("");
    req.set_prev_log_index(1);
    req.set_prev_log_term(1);
    req.set_leader_commit(1);

    log_entry temp;
    temp.set_term(1);
    temp.set_index(1);
    temp.set_log_data("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");
    temp.set_index(1);
    temp.set_type(e_raft_log);

    for (int i = 0; i < 10000; ++i)
    {
        log_entry *entry = new log_entry(temp);
        req.mutable_entries()->AddAllocated(entry);
    }
}

int main()
{
    int i =100000000;
    do{
        test_add_entry();
    }while(i--);

    logger("done");
    getchar();
}