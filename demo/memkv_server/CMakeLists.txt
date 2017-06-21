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
        ${memkv_proto_SOURCE_DIR}/include)

aux_source_directory(${memkv_server_SOURCE_DIR}/src memkv_server_sources)

add_executable(memkv_server ${memkv_server_sources})

target_link_libraries(memkv_server
        memkv_proto
        ${depend_libs})


