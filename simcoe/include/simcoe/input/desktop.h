#include "simcoe/input/input.h"
#include "simcoe/core/system.h"

#include <array>

namespace simcoe::input {
    struct Desktop final : ISource {
        using KeyState = std::array<size_t, Key::eTotal>;
        Desktop(bool capture);

        bool poll(State& result) override;

        void captureInput(bool capture);
        void updateKeys(UINT msg, WPARAM wparam, LPARAM lparam);
        void updateMouse(HWND hWnd);

        const KeyState& getKeys() const { return keys; }
        const math::float2& getMouse() const { return mouse; }
        bool isEnabled() const { return enabled; }

    private:
        void setKey(WORD key, size_t value);
        
        KeyState keys = {};
        math::float2 mouse = {};

        size_t index = 1;
        bool enabled;
    };
}
