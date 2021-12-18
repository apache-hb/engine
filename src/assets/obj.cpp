#include "loader.h"

#include "tinyobj.h"

namespace engine::loader {
    Model obj(std::string_view path) {
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn;
        std::string err;

        if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.data())) {
            throw LoadError(err, warn);
        }

        std::vector<Vertex> vertices;
        std::vector<DWORD> indicies;

        for (const auto& shape : shapes) {
            for (const auto& index : shape.mesh.indices) {
                auto vidx = index.vertex_index;
                auto tidx = index.texcoord_index;

                XMFLOAT3 vertex = XMFLOAT3(
                    attrib.vertices[3 * vidx + 2],
                    attrib.vertices[3 * vidx + 1],
                    attrib.vertices[3 * vidx + 0]
                );

                XMFLOAT2 texcoord = XMFLOAT2(
                    attrib.texcoords[2 * tidx + 0],
                    attrib.texcoords[2 * tidx + 1]
                );
                
                vertices.push_back(Vertex { vertex, texcoord });
                indicies.push_back(vertices.size() - 1);
            }
        }

        return { vertices, indicies };
    }
}
