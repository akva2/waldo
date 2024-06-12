cmake_minimum_required(VERSION 3.20)

project(waldo CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/3rd_party)

find_package(OpenMP REQUIRED)

include(FetchContent)
set(FETCHCONTENT_TRY_FIND_PACKAGE_MODE ALWAYS)

FetchContent_Declare(fmt
                     DOWNLOAD_EXTRACT_TIMESTAMP ON
                     URL https://github.com/fmtlib/fmt/archive/refs/tags/10.1.1.tar.gz
                     URL_HASH SHA512=288c349baac5f96f527d5b1bed0fa5f031aa509b4526560c684281388e91909a280c3262a2474d963b5d1bf7064b1c9930c6677fe54a0d8f86982d063296a54c)
FetchContent_MakeAvailable(fmt)

set(BUILD_GMOCK OFF CACHE INTERNAL "")
FetchContent_Declare(
    GTest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG f8d7d77c06936315286eb55f8de22cd23c188571) # release-1.14
FetchContent_MakeAvailable(GTest)

FetchContent_Declare(
    SDL2
    GIT_REPOSITORY  https://github.com/libsdl-org/SDL
    GIT_TAG         release-2.30.3
    GIT_PROGRESS    TRUE
)
FetchContent_MakeAvailable(sdl2)

set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "")
set(SDL2TTF_INSTALL ON CACHE INTERNAL "")
FetchContent_Declare(
    SDL2_ttf
    GIT_REPOSITORY  https://github.com/libsdl-org/SDL_ttf
    GIT_TAG         release-2.22.0
    GIT_PROGRESS    TRUE
)
FetchContent_MakeAvailable(SDL2_ttf)

FetchContent_declare(microstl
                     GIT_REPOSITORY https://github.com/cry-inc/microstl
                     GIT_TAG master)
FetchContent_Populate(microstl)

add_subdirectory(3rd_party/bvh)

add_library(Waldo src/ReadSTL.cpp
                  src/ReadSTL.hpp)

target_include_directories(Waldo PUBLIC ${microstl_SOURCE_DIR} 3rd_party/bvh)
target_link_libraries(Waldo PUBLIC fmt::fmt bvh)

add_executable(raytrace 3rd_party/bvh/camera.hpp
                        3rd_party/bvh/raytrace.cpp
                        3rd_party/bvh/raytrace.hpp)
target_include_directories(raytrace PUBLIC ${microstl_SOURCE_DIR} 3rd_party/bvh src)
if(WIN32)
  set_target_properties(raytrace PROPERTIES LINK_FLAGS /SUBSYSTEM:CONSOLE)
endif()

# Note: SDL2::SDL2main has to come before SDL2::SDL2
# https://github.com/msys2/MINGW-packages/issues/10459#issuecomment-1003700201
target_link_libraries(raytrace Waldo bvh SDL2::SDL2main SDL2::SDL2-static
                               SDL2_ttf::SDL2_ttf-static OpenMP::OpenMP_CXX)

add_executable(center_stl apps/center_stl.cpp)
target_include_directories(center_stl PUBLIC ${microstl_SOURCE_DIR})

set(TEST_SOURCES test/TestReadSTL.cpp)

add_executable(Waldo-test ${TEST_SOURCES})
target_link_libraries(Waldo-test Waldo GTest::gtest GTest::gtest_main)
target_include_directories(Waldo-test PUBLIC ${PROJECT_SOURCE_DIR}/src)

enable_testing()
include(GoogleTest)
gtest_discover_tests(Waldo-test
                     WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/test
                     NO_PRETTY_VALUES)
