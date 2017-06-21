project(memkv_proto)

cmake_minimum_required(VERSION 2.8)

#check acl path
if(NOT ACL_ROOT)
    message(FATAL_ERROR "need define ACL_ROOT")
endif()


include_directories(include
        ${ACL_ROOT}/lib_acl_cpp/include
        ${ACL_ROOT}/lib_acl/include)

aux_source_directory(${memkv_proto_SOURCE_DIR}/src memkv_proto_sources)
message(STATUS ${memkv_proto_sources})
add_library(memkv_proto ${memkv_proto_sources})

