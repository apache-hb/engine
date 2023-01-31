#include "simcoe/input/gamepad.h"
#include "simcoe/core/panic.h"

#include <xinput.h>

using namespace simcoe;
using namespace simcoe::input;

namespace {
    struct GamepadKeyMapping {
        Key slot;
        WORD mask;
    };

    constexpr GamepadKeyMapping kGamepadKeys[] = {
        { Key::padLeftBumper, XINPUT_GAMEPAD_LEFT_SHOULDER },
        { Key::padRightBumper, XINPUT_GAMEPAD_RIGHT_SHOULDER },
        { Key::padButtonDown, XINPUT_GAMEPAD_A },
        { Key::padButtonRight, XINPUT_GAMEPAD_B },
        { Key::padButtonLeft, XINPUT_GAMEPAD_X },
        { Key::padButtonUp, XINPUT_GAMEPAD_Y },
        { Key::padBack, XINPUT_GAMEPAD_BACK },
        { Key::padStart, XINPUT_GAMEPAD_START },
        { Key::padLeftStick, XINPUT_GAMEPAD_LEFT_THUMB },
        { Key::padRightStick, XINPUT_GAMEPAD_RIGHT_THUMB },
        { Key::padDirectionUp, XINPUT_GAMEPAD_DPAD_UP },
        { Key::padDirectionDown, XINPUT_GAMEPAD_DPAD_DOWN },
        { Key::padDirectionLeft, XINPUT_GAMEPAD_DPAD_LEFT },
        { Key::padDirectionRight, XINPUT_GAMEPAD_DPAD_RIGHT },
        { Key::padStart, XINPUT_GAMEPAD_START },
        { Key::padBack, XINPUT_GAMEPAD_BACK },
    };    
}

Gamepad::Gamepad(DWORD dwIndex)
    : ISource(Device::eGamepad)
    , dwIndex(dwIndex) 
{ }

bool Gamepad::poll(State& result) {
    XINPUT_STATE state;
    if (XInputGetState(dwIndex, &state) != ERROR_SUCCESS) {
        return false;
    }

    // TODO: this is wrong
    for (const auto& [slot, mask] : kGamepadKeys) {
        result.key[slot] = (state.Gamepad.wButtons & mask) != 0;
    }

    return true;
}
