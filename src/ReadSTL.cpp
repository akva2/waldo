/*
 *  Copyright (C) 2024 SINTEF Digital
 *
 *  SPDX-License-Identifier: GPL-3.0-or-later
 *  See LICENSE.md for more information.
 */

#include "bvh.hpp"
#include "include/microstl.h"

#include <fmt/format.h>

#include <filesystem>
#include <stdexcept>

namespace {
class BVHHandler : public microstl::Reader::Handler
{
public:
    void onFacetCount(uint32_t triangles) override
    {
        tris.reserve(triangles);
    }

    void onFacet(const float v1[3], const float v2[3],
                 const float v3[3], const float[3]) override
    {
        BVH::Triangle tri;
        tri.vertices[0] = Vector4(v1[0], v1[1], v1[2]);
        tri.vertices[1] = Vector4(v2[0], v2[1], v2[2]);
        tri.vertices[2] = Vector4(v3[0], v3[1], v3[2]);
        tris.emplace_back(std::move(tri));
    }

    std::vector<BVH::Triangle>&& triangles() { return std::move(tris); }

private:
    std::vector<BVH::Triangle> tris;
};

}

namespace STLReader {

std::vector<BVH::Triangle> read(std::string_view path, bool is_file)
{
    BVHHandler meshHandler;
    microstl::Result result;
    if (is_file) {
        std::filesystem::path fs_path(path);
        result = microstl::Reader::readStlFile(path, meshHandler);
    } else {
        result = microstl::Reader::readStlBuffer(path.data(), path.size(), meshHandler);
    }
    if (result != microstl::Result::Success)
        throw std::runtime_error(fmt::format("Error reading {}, error = {}",
                                             path, microstl::getResultString(result)));

    return meshHandler.triangles();
}

}
