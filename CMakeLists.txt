cmake_minimum_required(VERSION 2.8)

project(libraft)

if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
	message(FATAL_ERROR "please into build dir to build!")
endif()

if(NOT ACL_ROOT)
	message(FATAL_ERROR "libraft depend on acl.need define ACL_ROOT to build"
	"eg: cmake . -DACL_ROOT=/home/akzi/acl-dev/acl")
endif()

if(NOT HTTP_RPC_LIB_PATH)
	message(FATAL_ERROR "libraft depend on http_rpc.please define HTTP_RPC_LIB_PATH to build")
endif()

if(NOT HTTP_RPC_INCLUDE_PATH)
	message(FATAL_ERROR "libraft depend on http_rpc.please define HTTP_RPC_INCLUDE_PATH to build")
endif()

message(STATUS ${ACL_ROOT})
message(STATUS ${HTTP_RPC_INCLUDE_PATH})
message(STATUS ${HTTP_RPC_LIB_PATH})


include_directories(${ACL_ROOT}/lib_acl/include)
include_directories(${ACL_ROOT}/lib_acl_cpp/include)
include_directories(${HTTP_RPC_INCLUDE_PATH})
include_directories(${libraft_SOURCE_DIR}/include)
include_directories(${libraft_SOURCE_DIR}/include/proto_gen)

aux_source_directory(${libraft_SOURCE_DIR}/src SRC_LIST)

add_library(libraft ${SRC_LIST} src/proto_gen/raft.pb.cc)

add_subdirectory(demo)
