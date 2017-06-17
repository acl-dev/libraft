project(memkv_server)

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
        include
        ${ACL_ROOT}/lib_acl/include
        ${memkv_proto_SOURCE_DIR}/include
		${ACL_ROOT}/lib_acl_cpp/include
        ${HTTP_RPC_INCLUDE_PATH})

aux_source_directory(${memkv_server_SOURCE_DIR}/src memkv_server_sources)

message(STATUS "memkv_server_sources: ${memkv_server_sources}")

add_executable(memkv_server ${memkv_server_sources})



target_link_libraries(memkv_server
        libraft
        memkv_proto
        ${HTTP_RPC_LIB_PATH}/libhttp_rpc.a
        ${ACL_ROOT}/lib_acl_cpp/lib/lib_acl_cpp.a
        ${ACL_ROOT}/lib_protocol/lib/libprotocol.a
        ${ACL_ROOT}/lib_acl/lib/lib_acl.a
        protobuf
        pthread
        z)

