cmake_minimum_required(VERSION 3.20)

project(waldo CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# find_package(GTest REQUIRED)

include(FetchContent)

FetchContent_Declare(fmt
                     DOWNLOAD_EXTRACT_TIMESTAMP ON
                     URL https://github.com/fmtlib/fmt/archive/refs/tags/10.1.1.tar.gz
                     URL_HASH SHA512=288c349baac5f96f527d5b1bed0fa5f031aa509b4526560c684281388e91909a280c3262a2474d963b5d1bf7064b1c9930c6677fe54a0d8f86982d063296a54c)
FetchContent_Makeavailable(fmt)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG 58d77fa8070e8cec2dc1ed015d66b454c8d78850 # release-1.12.1
    OVERRIDE_FIND_PACKAGE)
FetchContent_MakeAvailable(googletest)

# find_package(fmt REQUIRED)


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
