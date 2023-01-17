#pragma once

#include <span>
#include <string>

namespace engine::debug {
    struct DebugObject;

    void addDebugObject(DebugObject* object);

    struct DebugObject {
        DebugObject(std::string&& name): name(name) { addDebugObject(this); }
        
        DebugObject(const DebugObject&) = delete;
        DebugObject(DebugObject&&) = delete;
        DebugObject& operator=(const DebugObject&) = delete;
        DebugObject& operator=(DebugObject&&) = delete;

        virtual ~DebugObject() = default;
        virtual void info() = 0;

        const std::string& getName() const { return name; }

    private:
        std::string name;
    };

    std::span<DebugObject*> getDebugObjects();
}
