#include "logging/debug.h"
#include "imgui.h"
#include "assets/world.h"

using namespace engine;
using namespace engine::assets;

struct WorldDebugObject : debug::DebugObject {
    using Super = debug::DebugObject;
    
    WorldDebugObject(World* world): Super("World"), world(world) {}

    virtual void info() override {
        ImGui::Text("World");
        for (size_t i = 0; i < world->indexBuffers.size(); i++) {
            const auto& buffer = world->indexBuffers[i];

            ImGui::Text("Index Buffer %zu. Size %zu", i, buffer.size());
            for (size_t j = 0; j < buffer.size(); j++) {
                ImGui::Text("\t%zu: %u", j, buffer[j]);
            }
        }
    }

private:
    World* world;
};

namespace engine::loader {
    void newDebugObject(assets::World* world) {
        new WorldDebugObject(world);
    }
}
