#include "debug.h"

#include <vector>

namespace engine::debug {
    std::vector<DebugObject*>& debugObjects() {
        static std::vector<DebugObject*> objects;
        return objects;
    }

    void addDebugObject(DebugObject* object) {
        debugObjects().push_back(object);
    }

    std::span<DebugObject*> getDebugObjects() {
        return debugObjects();
    }
}
