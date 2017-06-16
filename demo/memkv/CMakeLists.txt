project(memkv)

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

include_directories(${ACL_ROOT}/lib_acl/include)
include_directories(${ACL_ROOT}/lib_acl_cpp/include)
include_directories(${HTTP_RPC_INCLUDE_PATH})
include_directories(${memkv_SOURCE_DIR}/include)

aux_source_directory(${memkv_SOURCE_DIR}/src SRC_LIST)

add_executable(memkv ${SRC_LIST})



target_link_libraries(memkv 
	libraft 
	${HTTP_RPC_LIB_PATH}/libhttp_rpc.a
	${ACL_ROOT}/lib_acl_cpp/lib/lib_acl_cpp.a
	${ACL_ROOT}/lib_acl/lib/lib_acl.a
	${ACL_ROOT}/lib_protocol/lib/lib_protocol.a
	protobuf
	pthread
	z)

