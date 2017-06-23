## what is libraft ?
libraft base on [acl](https://github.com/acl-dev/acl) foundation framework 
implememt [Raft Consensus Algorithm](https://raft.github.io/).
with libraft you can build a high availability and data consistency system much easier.

## libraft  implementations

* Leader Election 
* Log Replication
* Log Compaction
* Membership Changes (todo)

## build it
* build acl
```bash
git clone https://github.com/acl-dev/acl.git
cd acl/
make build_one
```
* build http_rpc
```bash
git clone https://github.com/acl-dev/microservice.git
cd microservice/
mkdir build
cd build/
cmake .. -DACL_ROOT=$(pwd)/../../acl
make
```
* [build protobuf](https://github.com/google/protobuf/blob/master/src/README.md)

* build libraft
```bash
git clone https://github.com/acl-dev/libraft.git
cd libraft
./build.sh
```

## how to use libraft
to see the demo [memkv](https://github.com/acl-dev/libraft/tree/master/demo/)


## contact us
In use if you have any question, welcome feedback to me, you can use the following contact information to communicate with me

* email: niukey@qq.com
* QQ group :  242722074

