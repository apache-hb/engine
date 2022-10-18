#pragma once

#include "engine/base/logging.h"
#include "engine/base/macros.h"
#include "engine/base/util.h"
#include "engine/math/math.h"
#include "engine/base/win32.h"

#include <bitset>
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

    struct Toggle {
        Toggle(bool initial)
            : enabled(initial)
        { }

        bool update(size_t key) {
            if (key > last) {
                last = key;
                enabled = !enabled;
                return true;
            }

            return false;
        }

        bool get() const { return enabled; }
        void set(bool value) { 
            last = 0;
            enabled = value; 
        }
    private:
        size_t last = 0;
        bool enabled;
    };

    struct State {
        Device source { Device::eDesktop };
        size_t key[Key::eTotal] { };
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
        using KeyState = size_t[256];
        Keyboard();

        bool poll(State* pState) override;

        void captureInput(bool capture);
        void update(HWND hwnd, const KeyState state);
        bool isEnabled() const { return enabled; }

    private:
        bool enabled = true;

        KeyState keys;
        math::float2 cursor;
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
