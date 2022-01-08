#pragma once

#include "world.h"

namespace engine::loader {
    assets::World* gltfWorld(std::string_view path);
}
