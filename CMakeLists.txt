cmake_minimum_required(VERSION 2.8)

project(libraft)

include_directories(${libraft_SOURCE_DIR}/include)
include_directories(${ACL_ROOT}/lib_acl/include)
include_directories(${ACL_ROOT}/lib_acl_cpp/include)

aux_source_directory(${libraft_SOURCE_DIR}/src SRC_LIST)

add_library(libraft ${SRC_LIST})

add_subdirectory(demo)
