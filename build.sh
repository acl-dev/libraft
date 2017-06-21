#!/bin/bash
#this is a build tool
#U need to define ACL_ROOT ,HTTP_RPC_INCLUDE_PATH and HTTP_RPC_LIB_PATH
#to build it

cd protos/
./gen_proto.sh
cd ../
#rm -rf ./build
mkdir build
cd build
if [ -n $1 ]
then
    ACL_DEV=/home/skyinno/code
else
    ACL_DEV=$1
fi

#genarate make makefile
cmake .. -DACL_ROOT=${ACL_DEV}/acl \
-DHTTP_RPC_INCLUDE_PATH=${ACL_DEV}/microservice/http_rpc/include \
-DHTTP_RPC_LIB_PATH=${ACL_DEV}/microservice/build/http_rpc

# build 
make

#to do test libraft
cd bin

mkdir -p 11081/log 11081/metadata 11081/snapshot
mkdir -p 11082/log 11082/metadata 11082/snapshot

cp ../../demo/memkv_server/raft_config.json 11081/
cp ../../demo/memkv_server/raft_config.json 11082/

cp memkv_server 11081
cp memkv_server 11082


echo "#!/bin/bash" > 11082/run.sh
echo "cp ../memkv_server ." > 11082/run.sh
echo "./memkv_server alone 11082" > 11082/run.sh
chmod u+x 11082/run.sh

echo "#!/bin/bash" > 11081/run.sh
echo "cp ../memkv_server ." > 11081/run.sh
echo "./memkv_server alone" > 11081/run.sh
chmod u+x 11081/run.sh

echo "ok!"
