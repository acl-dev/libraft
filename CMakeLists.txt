cmake_minimum_required(VERSION 2.8)

project(libraft)

if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
	message(FATAL_ERROR "please into build dir to build!")
endif()

if(NOT ACL_ROOT)
	message(FATAL_ERROR "libraft depend on acl.need define ACL_ROOT to build"
	"eg: cmake .. -DACL_ROOT=/home/akzi/acl-dev/acl")
endif()

if(NOT HTTP_RPC_LIB_PATH)
	message(FATAL_ERROR "libraft depend on http_rpc."
                        "please define HTTP_RPC_LIB_PATH to build")
endif()

if(NOT HTTP_RPC_INCLUDE_PATH)
	message(FATAL_ERROR "libraft depend on http_rpc."
                        "please define HTTP_RPC_INCLUDE_PATH to build")
endif()

if(CMAKE_CL_64)
	set(ADDRESS_MODEL 64)
	set(NODE_TARGET x64)
else()
	set(ADDRESS_MODEL 32)
	set(NODE_TARGET x86)
endif()

message(STATUS "ACL_ROOT : ${ACL_ROOT}")
message(STATUS "HTTP_RPC_INCLUDE_PATH : ${HTTP_RPC_INCLUDE_PATH}")
message(STATUS "HTTP_RPC_LIB_PATH : ${HTTP_RPC_LIB_PATH}")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include_directories(${ACL_ROOT}/lib_acl/include 
					${ACL_ROOT}/lib_acl_cpp/include
					${HTTP_RPC_INCLUDE_PATH}
					${libraft_SOURCE_DIR}/include
					${libraft_SOURCE_DIR}/include/proto_gen
					${protobuf_include_path})

aux_source_directory(${libraft_SOURCE_DIR}/src libraft_sources)

add_library(libraft ${libraft_sources} src/proto_gen/raft.pb.cc)

message(STATUS "system name : ${CMAKE_SYSTEM_NAME}")


if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
	message(STATUS "--------${Linux}-------")
	set(depend_libs
			libraft
			${HTTP_RPC_LIB_PATH}/libhttp_rpc.a
			${ACL_ROOT}/lib_acl_cpp/lib/lib_acl_cpp.a
			${ACL_ROOT}/lib_protocol/lib/libprotocol.a
			${ACL_ROOT}/lib_acl/lib/lib_acl.a
			${protobuf_libs}
			protobuf
			pthread
			z)
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
	message(STATUS "--------{Windows}-------")
	#you need define these configs for you self 
	set(protobuf_lib_path F:/fork/akzi/acl-dev/protobuf/cmake/build/solution/Debug)
	include_directories(F:/fork/akzi/acl-dev/protobuf/include)
	set(depend_libs 
		libraft
		${HTTP_RPC_LIB_PATH}/libhttp_rpc.lib
		${ACL_ROOT}/lib_acl_cpp/lib_acl_cpp_vc2015d.lib
		${ACL_ROOT}/lib_acl/lib_acl_vc2015d.lib
		${ACL_ROOT}/lib_protocol/lib_protocol_vc2015d.lib
		${protobuf_lib_path}/libprotocd.lib
		${protobuf_lib_path}/libprotobufd.lib)
endif()

add_subdirectory(demo)

add_subdirectory(tests)
