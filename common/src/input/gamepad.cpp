#include "engine/input/input.h"

using namespace simcoe;
using namespace simcoe::input;

///
/// gamepad support via xinput
///

namespace {
    constexpr UINT kLeftDeadzone = XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE;
    constexpr UINT kRightDeadzone = XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE;
    constexpr float kTriggerDeadzone = XINPUT_GAMEPAD_TRIGGER_THRESHOLD;

    bool stickInsideDeadzone(float x, float y, float deadzone) {
        return sqrt((x * x) + (y * y)) < deadzone;
    }

    constexpr float triggerRatio(float it, float deadzone) {
        if (it > deadzone) {
            return it / 255.f;
        }

        return 0.f;
    }

    struct GamepadKeyMapping {
        Key::Slot slot;
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
    : Source(Device::eGamepad)
    , dwIndex(dwIndex) 
{ }

bool Gamepad::poll(State *pState) {
    XINPUT_STATE state;
    if (XInputGetState(dwIndex, &state) != ERROR_SUCCESS) {
        return false;
    }

    // write new axis state
    
    if (stickInsideDeadzone(state.Gamepad.sThumbLX, state.Gamepad.sThumbLY, kLeftDeadzone)) {
        pState->axis[Axis::padLeftStickVertical] = 0.f;
        pState->axis[Axis::padLeftStickHorizontal] = 0.f;
    } else {
        pState->axis[Axis::padLeftStickVertical] = float(state.Gamepad.sThumbLY) / SHRT_MAX;
        pState->axis[Axis::padLeftStickHorizontal] = float(state.Gamepad.sThumbLX) / SHRT_MAX;
    }

    if (stickInsideDeadzone(state.Gamepad.sThumbRX, state.Gamepad.sThumbRY, kRightDeadzone)) {
        pState->axis[Axis::padRightStickVertical] = 0.f;
        pState->axis[Axis::padRightStickHorizontal] = 0.f;
    } else {
        pState->axis[Axis::padRightStickVertical] = float(state.Gamepad.sThumbRY) / SHRT_MAX;
        pState->axis[Axis::padRightStickHorizontal] = float(state.Gamepad.sThumbRX) / SHRT_MAX;
    }

    /// triggers
    pState->axis[Axis::padLeftTrigger] = triggerRatio(state.Gamepad.bLeftTrigger, kTriggerDeadzone);
    pState->axis[Axis::padRightTrigger] = triggerRatio(state.Gamepad.bRightTrigger, kTriggerDeadzone);

    for (const auto& [slot, mask] : kGamepadKeys) {
        pState->key[slot] = (state.Gamepad.wButtons & mask) != 0;
    }

    return true;
}
