cmake_minimum_required(VERSION 3.20)

project(waldo CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(fmt REQUIRED)
find_package(GTest REQUIRED)

include(FetchContent)

FetchContent_declare(microstl
                     GIT_REPOSITORY https://github.com/cry-inc/microstl
                     GIT_TAG master)
FetchContent_Populate(microstl)

add_subdirectory(3rd_party/bvh)

add_library(Waldo src/ReadSTL.cpp
                  src/ReadSTL.hpp)

target_include_directories(Waldo PUBLIC ${microstl_SOURCE_DIR} 3rd_party/bvh)
target_link_libraries(Waldo PUBLIC fmt::fmt)

set(TEST_SOURCES test/TestReadSTL.cpp)

add_executable(Waldo-test ${TEST_SOURCES})
target_link_libraries(Waldo-test Waldo GTest::gtest GTest::gtest_main)
target_include_directories(Waldo-test PUBLIC ${PROJECT_SOURCE_DIR}/src)

enable_testing()
include(GoogleTest)
gtest_discover_tests(Waldo-test
                     WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/test
                     NO_PRETTY_VALUES)
