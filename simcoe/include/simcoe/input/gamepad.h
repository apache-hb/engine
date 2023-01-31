#include "simcoe/input/input.h"
#include "simcoe/core/win32.h"

namespace simcoe::input {
    struct Gamepad final : ISource {
        Gamepad(DWORD dwIndex);

        bool poll(State& result) override;

    private:
        DWORD dwIndex;
    };
}
