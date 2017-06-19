


add_executable(mmlog_manager_test mmlog_manager_test/main.cpp)
target_link_libraries(mmlog_manager_test
        ${depend_libs})

add_executable(mmlog_test mmlog_test/main.cpp)
target_link_libraries(mmlog_test
        ${depend_libs})

add_executable(metadata_test metadata_test/main.cpp)
target_link_libraries(metadata_test
        ${depend_libs})