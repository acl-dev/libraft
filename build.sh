#!/bin/bash
#this is a build tool
#U need to define ACL_ROOT ,HTTP_RPC_INCLUDE_PATH and HTTP_RPC_LIB_PATH
#to build it

cd protos/
#remove old protoc genarate files.
rm raft.pb.*
./gen_proto.sh
cd ../
rm -rf ./build
mkdir build
cd build
if [ -n $1 ]
then
    ACL_DEV=/home/akzi/code/acl-dev/
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


cp memkv_server 11081
cp memkv_server 11082

echo "{">>11081/raft_config.json
echo "	\"log_path\": \"log/\",">>11081/raft_config.json
echo "	\"snapshot_path\":\"snapshot/\",">>11081/raft_config.json
echo "	\"metadata_path\":\"metadata/\",">>11081/raft_config.json
echo "	\"max_log_size\": 1024,"   >>11081/raft_config.json
echo "	\"max_log_count\": 6,"     >>11081/raft_config.json
echo "	\"peer_addrs\": [" >>11081/raft_config.json
echo "		{">>11081/raft_config.json
echo "			\"addr\": \"127.0.0.1:11082\",">>11081/raft_config.json
echo "			\"id\"  :\"11082\"">>11081/raft_config.json
echo "	 	}">>11081/raft_config.json
echo "	],">>11081/raft_config.json
echo "	\"node_addr\": {">>11081/raft_config.json
echo "	\"addr\":\"127.0.0.1:11081\",">>11081/raft_config.json
echo "	\"id\": \"11081\"">>11081/raft_config.json
echo "	}">>11081/raft_config.json
echo "}">>11081/raft_config.json


echo "{">>11082/raft_config.json
echo "	\"log_path\": \"log/\",">>11082/raft_config.json
echo "	\"snapshot_path\":\"snapshot/\",">>11082/raft_config.json
echo "	\"metadata_path\":\"metadata/\",">>11082/raft_config.json
echo "	\"max_log_size\": 1024,"   >>11082/raft_config.json
echo "	\"max_log_count\": 6,"     >>11082/raft_config.json
echo "	\"peer_addrs\": [" >>11082/raft_config.json
echo "		{">>11082/raft_config.json
echo "			\"addr\": \"127.0.0.1:11081\",">>11082/raft_config.json
echo "			\"id\"  :\"11081\"">>11082/raft_config.json
echo "   	}">>11082/raft_config.json
echo "	],">>11082/raft_config.json
echo "	\"node_addr\": {">>11082/raft_config.json
echo "	\"addr\":\"127.0.0.1:11082\",">>11082/raft_config.json
echo "	\"id\": \"11082\"">>11082/raft_config.json
echo "	}">>11082/raft_config.json
echo "}">>11082/raft_config.json



echo "#!/bin/bash" >> 11082/run.sh
echo "cp ../memkv_server ." >> 11082/run.sh
echo "./memkv_server alone 11082" >> 11082/run.sh
chmod u+x 11082/run.sh

echo "#!/bin/bash" >> 11081/run.sh
echo "cp ../memkv_server ." >> 11081/run.sh
echo "./memkv_server alone" >> 11081/run.sh
chmod u+x 11081/run.sh

echo "ok!"
