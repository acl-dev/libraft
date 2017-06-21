## memkv
memkv will show you how to build fault-tolerance project
###### [memkv_server](https://github.com/acl-dev/libraft/tree/master/demo/memkv_server)
memkv_server will load config from [raft_config.json](https://github.com/acl-dev/libraft/blob/master/demo/memkv_server/raft_config.json)
```json
{
    "log_path": "log/",
    "snapshot_path": "snapshot/",
    "metadata_path": "metadata/",
    "max_log_size": 1024,
    "max_log_count": 6,
    "peer_addrs": [
        {
            "addr": "127.0.0.1:11082",
            "id": "11082"
        }
    ],
    "node_addr": {
        "addr": "127.0.0.1:11081",
        "id": "11081"
    }
}
```
###### log_path
memkv_server to store log request from memkv_client.it is [raft log](https://raft.github.io/)
###### snapshot_path
when libraft to do log compaction,it maybe will make snapshot of the memkv state.and write snapshot file in this path
###### metadata_path  
libraft will write some metadata(applied_index_, committed_index_,vote_for_,current_term_) to a metadata file.
and it will create metafile in this path.
###### max_log_size
if one log file size greater than this, libraft will create new log file to write log entries.
###### max_log_count
if the count of all the log files greater than this, libraft will do log compaction to discard some log files
###### peer_addr
* addr: the addresses of peer node.
* id  :  unique id to identify raft node.

#### node_addr
the address of this node.


## run memkv_server
```bash
./memkv_server alone 127.0.0.1:11081
```
127.0.1:11081 is the node bind address

###### [memkv_proto](https://github.com/acl-dev/libraft/tree/master/demo/memkv_proto)
memkv use json protocol to serialization request and response.and it use [Gson](https://github.com/acl-dev/acl/tree/master/app/gson) to genarate serialization cpp codes.

###### [memkv_client](https://github.com/acl-dev/libraft/tree/master/demo/memkv_client)

when memkv_client start.it will load cluster_config.json file. and get memkv_server cluster addresses.
so before start memkv_client. you should make sure cluster_config.json eixst.
```json
{
    "addrs":[
        {"addr":"127.0.0.1:11081","id":"11081"},
        {"addr":"127.0.0.1:11082","id":"11082"}
    ]
}
```
###### addrs
addresses of this memkv_cluster. it same with raft_config.json addr

* id : raft unique id.it same with raft_config.json
