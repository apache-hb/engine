#pragma once

#include <span>
#include <string_view>

namespace engine::rhi {
    struct Device {
        virtual ~Device() = default;
    };

    struct Adapter {
        enum Type {
            eDiscrete,
            eSoftare,
            eOther
        };

        virtual std::string_view getName() const = 0;
        virtual Type getType() const = 0;

        virtual ~Adapter() = default;
    };

    struct Instance {
        Adapter *getBestAdapter();
        
        virtual Device *newDevice(Adapter *adapter) = 0;

        virtual std::span<Adapter*> getAdapters() = 0;

        virtual ~Instance() = default;
    };

    Instance *getInstance();
}
