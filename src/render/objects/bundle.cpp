#include "bundle.h"

namespace engine::render {
    void CommandBundle::tryDrop(std::string_view name) {
        Super::tryDrop(name);
        allocator.tryDrop(std::format("{}-allocator", name));
    }
}
