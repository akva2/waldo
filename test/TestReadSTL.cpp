/*
 *  Copyright (C) 2024 SINTEF Digital
 *
 *  SPDX-License-Identifier: GPL-3.0-or-later
 *  See LICENSE.md for more information.
 */

#include <gtest/gtest.h>

#include "bvh.hpp"

#include "ReadSTL.hpp"

namespace {

const std::string_view two_triangles = R"(
solid twotriangles
    facet normal 0.0 0.0 1.0
        outer loop
            vertex 0.0 0.5 0.0
            vertex 0.5 0.0 0.0
            vertex 0.0 0.0 0.0
        endloop
    endfacet
    facet normal 0.0 0.0 1.0
        outer loop
            vertex 0.5 0.5 0.0
            vertex 0.5 0.0 0.0
            vertex 0.0 0.5 0.0
        endloop
    endfacet
endsolid twotriangles)";

}

TEST(TestReadSTL, TwoTriangles)
{
    try {
        const auto [tris, normals] = STLReader::read(two_triangles, false);
        EXPECT_EQ(tris.size(), 2);
        EXPECT_EQ(tris[0].vertices[0].x, 0.0);
        EXPECT_EQ(tris[0].vertices[0].y, 0.5);
        EXPECT_EQ(tris[0].vertices[0].z, 0.0);
        EXPECT_EQ(tris[0].vertices[1].x, 0.5);
        EXPECT_EQ(tris[0].vertices[1].y, 0.0);
        EXPECT_EQ(tris[0].vertices[1].z, 0.0);
        EXPECT_EQ(tris[0].vertices[2].x, 0.0);
        EXPECT_EQ(tris[0].vertices[2].y, 0.0);
        EXPECT_EQ(tris[0].vertices[2].z, 0.0);
        EXPECT_EQ(tris[1].vertices[0].x, 0.5);
        EXPECT_EQ(tris[1].vertices[0].y, 0.5);
        EXPECT_EQ(tris[1].vertices[0].z, 0.0);
        EXPECT_EQ(tris[1].vertices[1].x, 0.5);
        EXPECT_EQ(tris[1].vertices[1].y, 0.0);
        EXPECT_EQ(tris[1].vertices[1].z, 0.0);
        EXPECT_EQ(tris[1].vertices[2].x, 0.0);
        EXPECT_EQ(tris[1].vertices[2].y, 0.5);
        EXPECT_EQ(tris[1].vertices[2].z, 0.0);
    } catch (std::exception& e) {
        std::cerr << "Caught exception " << e.what() << std::endl;
    }
}
