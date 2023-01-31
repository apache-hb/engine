#pragma once

#include "simcoe/math/math.h"

#include <vector>

namespace simcoe::input {
    namespace KeyTags {
        enum Slot : unsigned {
#define KEY(id, name) id,
#include "simcoe/input/input.inc"
            eTotal
        };
    }

    namespace AxisTags {
        enum Slot : unsigned {
#define AXIS(id, name) id,
#include "simcoe/input/input.inc"
            eTotal
        };
    }

    namespace DeviceTags {
        enum Slot : unsigned {
#define DEVICE(id, name) id,
#include "simcoe/input/input.inc"
            eTotal
        };
    }

    using Key = KeyTags::Slot;
    using Axis = AxisTags::Slot;
    using Device = DeviceTags::Slot;

    struct State final {
        Device device;
        size_t key[Key::eTotal];
        float axis[Axis::eTotal];
    };

    struct ISource {
        ISource(Device kind);

        virtual ~ISource() = default;
        virtual bool poll(State& result) = 0;

        const Device kind;
    };

    struct ITarget {
        virtual ~ITarget() = default;
        virtual void accept(const State& state) = 0;
    };

    struct Manager final {
        void poll();

        void add(ISource* source);
        void add(ITarget* target);

    private:
        std::vector<ISource*> sources;
        std::vector<ITarget*> targets;
        State last = {};
    };
}
