#pragma once

#include "simcoe/core/util.h"
#include "simcoe/input/input.h"
#include "simcoe/core/win32.h"

namespace simcoe::input {
    struct Gamepad final : ISource {
        Gamepad(DWORD dwIndex);

        bool poll(State& result) override;

    private:
        bool updateKey(State& result, Key key, WORD mask, WORD state);
        
        DWORD dwIndex;
        size_t index = 1;
        
        util::DoOnce reportNotConnected;
    };
}
