#pragma once

#include "simcoe/math/math.h"

#include <vector>

namespace simcoe::input {
    namespace Key {
        enum Slot : unsigned {
            eTotal
        };
    }

    namespace Axis {
        enum Slot : unsigned {
            eTotal
        };
    }

    enum struct Device {
        eDesktop,
        eGamepad
    };

    struct State {
        Device device;
        size_t key[Key::eTotal];
        float axis[Axis::eTotal];
    };

    struct ISource {
        ISource(Device kind);

        virtual ~ISource() = default;
        virtual bool poll(State& state) = 0;

        const Device kind;
    };

    struct ITarget {
        virtual ~ITarget() = default;
        virtual void accept(const State& state) = 0;
    };

    struct Manager {
        void poll();

        void add(ISource* source);
        void add(ITarget* target);

    private:
        std::vector<ISource*> sources;
        std::vector<ITarget*> targets;
        State last;
    };
}
