#include "include/microstl.h"

#include <iostream>

int main(int argc, char** argv)
{
    microstl::Reader r;
    microstl::MeshReaderHandler handler;
    r.readStlFile(argv[1], handler);

    std::array<float,3> center{};
    const size_t nf = handler.mesh.facets.size();
    for (const microstl::Facet& f : handler.mesh.facets)
        for (const auto& v : {f.v1, f.v2, f.v3}) {
            center[0] += v.x / (3.f*nf);
            center[1] += v.y / (3.f*nf);
            center[2] += v.z / (3.f*nf);
        }

    std::cout << "center: "
              << center[0]
              << " " << center[1]
              << " " << center[2] << std::endl;

    for (microstl::Facet& f : handler.mesh.facets) {
        for (auto& v : {&f.v1, &f.v2, &f.v3}) {
            v->x -= center[0];
            v->y -= center[1];
            v->z -= center[2];
        }
    }

    float scale = -1.0;
    for (const microstl::Facet& f : handler.mesh.facets)
        for (const auto& v : {f.v1, f.v2, f.v3}) {
            scale = std::max(std::fabs(v.x), scale);
            scale = std::max(std::fabs(v.y), scale);
            scale = std::max(std::fabs(v.z), scale);
        }

    std::cout << "scale: " << scale << std::endl;

    for (microstl::Facet& f : handler.mesh.facets) {
        for (auto& v : {&f.v1, &f.v2, &f.v3}) {
            v->x /= scale;
            v->y /= scale;
            v->z /= scale;
        }
    }

    microstl::Writer w;
    microstl::MeshProvider provider(handler.mesh);
    w.writeStlFile(argv[2], provider);
    return 0;
}
