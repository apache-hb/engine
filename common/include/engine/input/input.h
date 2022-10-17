#pragma once

#include "engine/base/macros.h"
#include "engine/base/util.h"
#include "engine/math/math.h"
#include "engine/base/win32.h"

#include <xinput.h>

#include <vector>

namespace simcoe::input {
    namespace Key {
        enum Slot : unsigned {
#define KEY(id, name) id,
#include "engine/input/input.inl"

            eTotal
        };
    }

    namespace Axis {
        enum Slot : unsigned {
#define AXIS(id, name) id,
#include "engine/input/input.inl"

            eTotal
        };
    }

    enum struct Device {
#define DEVICE(id, name) id,
#include "engine/input/input.inl"

        eTotal
    };

    struct State {
        Device source { Device::eDesktop };
        bool key[Key::eTotal] { };
        float axis[Axis::eTotal] { };
    };

    // an input event source
    struct Source {
        Source(Device kind) : kind(kind) { }

        virtual ~Source() = default;
        virtual bool poll(State* pState) = 0;

        const Device kind;
    };

    // an input event listener
    struct Listener {
        virtual ~Listener() = default;
        virtual bool update(const State& state) = 0;
    };

    using ListenerVec = std::vector<Listener*>;
    using SourceVec = std::vector<Source*>;

    /**
     * input system lifecycle
     *
     * 1. create input manager
     *    - add input listeners
     *    - add input sources
     * 2. poll on input manager in new thread
     *    - input manager polls sources and pumps events to listeners
     */

    // input event manager
    struct Manager {
        Manager(SourceVec sources, ListenerVec listeners)
            : sources(sources)
            , listeners(listeners)
        { }

        void poll();

        void addListener(Listener *pListener);
        void addSource(Source *pSource);
    private:
        SourceVec sources;
        ListenerVec listeners;
        State prevState;
    };

    // input sources

    struct Keyboard final : Source {
        Keyboard();

        bool poll(State* pState) override;

        void update();
    private:
    };

    struct Gamepad final : Source {
        Gamepad(DWORD dwIndex);
        
        bool poll(State* pState) override;

    private:
        // controller index
        DWORD dwIndex;
    };

    // input listeners

    struct DebugListener final : Listener {
        bool update(const State& state) override;
    };
}

namespace simcoe::locale {
    std::string_view get(input::Device device);
    std::string_view get(input::Key::Slot key);
    std::string_view get(input::Axis::Slot axis);
}
