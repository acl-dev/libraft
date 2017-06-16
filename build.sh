#!/bin/bash -x
#this is a build tool
#U need to define ACL_ROOT ,HTTP_RPC_INCLUDE_PATH and HTTP_RPC_LIB_PATH
#to build it

cd protos/
./gen_proto.sh
cd ../
rm -rf ./build
mkdir build
cd build
ACL_DEV=/home/akzi/code/acl-dev

cmake .. -DACL_ROOT=${ACL_DEV}/acl \
-DHTTP_RPC_INCLUDE_PATH=${ACL_DEV}/microservice/http_rpc/include \
-DHTTP_RPC_LIB_PATH=${ACL_DEV}/microservice/build/http_rpc
make

