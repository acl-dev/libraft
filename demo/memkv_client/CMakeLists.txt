
project(memkv_client)

cmake_minimum_required(VERSION 2.8)

include_directories(${memkv_proto_SOURCE_DIR}/include
					${memkv_client_SOURCE_DIR}/include)

aux_source_directory(${memkv_client_SOURCE_DIR}/src SRC_LIST)

add_executable(memkv_client ${SRC_LIST})

target_link_libraries(memkv_client
        memkv_proto
        ${depend_libs})


