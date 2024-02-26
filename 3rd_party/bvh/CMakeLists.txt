cmake_minimum_required(VERSION 3.16)

project(BVH)

find_package(OpenMP REQUIRED)

add_library(bvh bvh.cpp bvh.hpp subdivision.hpp ray_intersection.hpp utils.hpp non_copyable.hpp)
