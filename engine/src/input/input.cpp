#include "engine/input/input.h"

#include "engine/base/win32.h"
#include "xinput.h"

using namespace engine;

namespace {
    constexpr UINT kLeftDeadzone = XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE;
    constexpr UINT kRightDeadzone = XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE;

    math::float2 stickRatio(float x, float y, float deadzone) {
        float magnitude = sqrt((x * x) + (y * y));

        if (magnitude > deadzone) {
            return math::float2 { x / SHRT_MAX, y / SHRT_MAX };
        } else {
            return math::float2::of(0.f);
        }
    }
}

input::Input input::poll() {
    XINPUT_STATE state;
    if (DWORD result = XInputGetState(0, &state); result != ERROR_SUCCESS) {
        return { };
    }

    return input::Input {
        .movement = stickRatio(state.Gamepad.sThumbLX, state.Gamepad.sThumbLY, kLeftDeadzone).vec3(0.f),
        .rotation = stickRatio(state.Gamepad.sThumbRX, state.Gamepad.sThumbRY, kRightDeadzone)
    };
}
