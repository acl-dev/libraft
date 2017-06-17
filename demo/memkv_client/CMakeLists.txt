
project(memkv_client)

cmake_minimum_required(VERSION 2.8)

#check http_rpc lib path
if(NOT HTTP_RPC_LIB_PATH)
    message(FATAL_ERROR "need define HTTP_RPC_LIB_PATH")
endif()

#check http_rpc include path
if(NOT HTTP_RPC_INCLUDE_PATH)
    message(FATAL_ERROR "need define HTTP_RPC_LIB_INCLUDE")
endif()

#check acl path
if(NOT ACL_ROOT)
    message(FATAL_ERROR "need define ACL_ROOT")
endif()

message(STATUS "memkv_proto_include:${memkv_proto_SOURCE_DIR}/include")

include_directories(
        ${memkv_proto_SOURCE_DIR}/include
        include
        ${ACL_ROOT}/lib_acl/include
        ${ACL_ROOT}/lib_acl_cpp/include
        ${ACL_ROOT}/lib_protocol/include
        ${HTTP_RPC_INCLUDE_PATH})

aux_source_directory(${memkv_client_SOURCE_DIR}/src SRC_LIST)

add_executable(memkv_client ${SRC_LIST})

target_link_libraries(memkv_client
        memkv_proto
        ${HTTP_RPC_LIB_PATH}/libhttp_rpc.a
        ${ACL_ROOT}/lib_acl_cpp/lib/lib_acl_cpp.a
        ${ACL_ROOT}/lib_protocol/lib/libprotocol.a
        ${ACL_ROOT}/lib_acl/lib/lib_acl.a
        pthread
        z)


