#include "simcoe/input/input.h"

#include <array>

namespace simcoe::input {
    struct Desktop final : ISource {
        using KeyState = std::array<size_t, 256>;
        Desktop();

        bool poll(State& state) override;

        void update(HWND hWnd, const KeyState& state);

    private:
        KeyState keys;
        math::float2 mouse;
    };
}
