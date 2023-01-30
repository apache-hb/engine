#include "simcoe/input/input.h"

#include <xinput.h>

namespace simcoe::input {
    struct Gamepad final : ISource {
        Gamepad(DWORD dwIndex);

        bool poll(State& state) override;
    private:
        XINPUT_GAMEPAD state;
    };
}
