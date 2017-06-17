
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

include_directories(
        ${ACL_ROOT}/lib_acl/include
        ${ACL_ROOT}/lib_acl_cpp/include
        ${ACL_ROOT}/lib_protocol/include
        ${HTTP_RPC_INCLUDE_PATH}
        ${memkv_client_SROURCE_DIR}/include)

aux_source_directory(${memkv_client_SOURCE_DIR}/src SRC_LIST)

add_executable(memkv_client ${SRC_LIST})

target_link_libraries(memkv_client
        ${HTTP_RPC_LIB_PATH}/http_rpc
        ${ACL_ROOT}/lib_acl_cpp/lib/lib_acl_cpp.a
        $(ACL_ROOT)/lib_protocol/lib/lib_protocol.a
        ${ACL_ROOT}/lib_acl/lib/lib_acl.a
        pthread
        z)


