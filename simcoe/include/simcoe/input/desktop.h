#include "simcoe/input/input.h"
#include "simcoe/core/system.h"

#include <array>

namespace simcoe::input {
    struct Keyboard final : ISource {
        using KeyState = std::array<size_t, Key::eTotal>;
        Keyboard();

        bool poll(State& result) override;

        void update(UINT msg, WPARAM wparam, LPARAM lparam);

    private:
        void setKey(WORD key, size_t value);
        void setXButton(WORD key, size_t value);
        
        KeyState keys = {};

        size_t index = 1;
    };

    struct Mouse final : ISource {
        Mouse(bool lockCursor, bool readCursor);

        bool poll(State& result) override;

        void captureInput(bool capture);
        void update(HWND hWnd);

    private:
        // the position that the mouse is relative to
        // either the center of the screen or the last
        // position of the mouse
        math::float2 base = {};

        // the absolute position of the mouse
        math::float2 absolute = {};

        /// is the mouse currently captured (hidden and locked to the center of the screen)
        bool captured;

        /// is the mouse currently enabled (should it be read)
        bool enabled;
    };
}
