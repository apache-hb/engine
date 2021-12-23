#include "loader.h"

namespace engine::loader {
    gltf::Model gltfScene(std::string_view path) {
        gltf::Model model;
        gltf::TinyGLTF loader;
        std::string err;
        std::string warn;

        bool res = loader.LoadASCIIFromFile(&model, &err, &warn, path.data());
        if (!res) { throw LoadError(err, warn); }

        return model;
    }
}
