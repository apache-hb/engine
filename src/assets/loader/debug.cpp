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
    }

private:
    World* world;
};

namespace engine::loader {
    void newDebugObject(assets::World* world) {
        new WorldDebugObject(world);
    }
}
