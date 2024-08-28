#ifndef BVH_HPP_
#define BVH_HPP_

#include <cstddef>
#include <vector>

#include "non_copyable.hpp"
#include "vec4.hpp"

namespace BVH
{

    struct Triangle
    {
        Vector4 vertices[3];

        Vector4 calc_centroid() const
        {
            return (vertices[0] + vertices[1] + vertices[2]) / 3;
        }
    };

    struct AABB
    {
        Vector4 upper, lower;
    };

    struct Node
    {
        std::vector<Triangle>::iterator begin, end;
        Node *left = nullptr, *right = nullptr;
        AABB aabb;

        Node() = default;

        Node(std::vector<Triangle>::iterator begin, std::vector<Triangle>::iterator end)
        {
            this->begin = begin;
            this->end = end;
            left = nullptr;
            right = nullptr;
        }

        bool is_leaf() const
        {
            return (left == nullptr) && (right == nullptr);
        }
    };

    class AABBTree : public NonCopyable
    {

    private:
        std::vector<Triangle>& tris;
        std::vector<Node> preallocated_nodes;
        std::size_t num_used_nodes = 0;

        Node *new_node(std::vector<Triangle>::iterator begin, std::vector<Triangle>::iterator end);
        void subdivide(Node *, float);

    public:
        Node* root = nullptr;

        AABBTree(std::vector<Triangle>& tris, float aabb_expansion);

        bool does_intersect_ray(Vector4 origin, Vector4 direction, float *t_out, Vector4 *pt_out, Vector4 *normal_out) const;

        void print_stats() const;
    };

}

#endif
