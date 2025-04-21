cmake_minimum_required(VERSION 3.14)

# include(gtest)
include(GoogleTest)
link_directories(${CMAKE_ARCHIVE_OUTPUT_DIRECTORY})

file(GLOB_RECURSE LOCK_TEST_SOURCES "${PROJECT_SOURCE_DIR}/test/*test.cpp")

# "make all-tests"
add_custom_target(show-tests COMMAND ${CMAKE_CTEST_COMMAND} --show-only)
add_custom_target(all-tests COMMAND ${CMAKE_CTEST_COMMAND} --verbose)

# "make XXX_test"
foreach(lock_test_source ${LOCK_TEST_SOURCES})
    get_filename_component(lock_test_filename ${lock_test_source} NAME)
    string(REPLACE ".cpp" "" lock_test_name ${lock_test_filename})

    add_executable(${lock_test_name} EXCLUDE_FROM_ALL ${lock_test_source})
    add_dependencies(show-tests ${lock_test_name})
    add_dependencies(all-tests ${lock_test_name})

    gtest_discover_tests(${lock_test_name}
            DISCOVERY_MODE PRE_TEST
            EXTRA_ARGS
            --gtest_catch_exceptions=0
            DISCOVERY_TIMEOUT 120
            PROPERTIES
            TIMEOUT 120
            )

    target_link_libraries(${lock_test_name} lock gtest gtest_main pthread)

    # Set test target properties
    set_target_properties(${lock_test_name}
            PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/test"
            COMMAND ${lock_test_name}
            )

endforeach()

