#include "simcoe/input/gamepad.h"
#include "simcoe/core/panic.h"
#include "simcoe/core/system.h"

#include "simcoe/simcoe.h"

#include "common.h"

#include <xinput.h>

using namespace simcoe;
using namespace simcoe::input;

namespace {
    constexpr UINT kLeftDeadzone = XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE;
    constexpr UINT kRightDeadzone = XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE;
    constexpr float kTriggerDeadzone = XINPUT_GAMEPAD_TRIGGER_THRESHOLD;

    bool stickInsideDeadzone(float x, float y, float deadzone) {
        return sqrt((x * x) + (y * y)) < deadzone;
    }

    constexpr float triggerRatio(float it, float deadzone, bool& dirty) {
        if (it > deadzone) {
            dirty = true;
            return it / 255.f;
        }

        return 0.f;
    }

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
{ 
    gInputLog.info("creating xinput gamepad for user {}", dwIndex);
}

bool Gamepad::poll(State& result) {
    bool dirty = false;
    XINPUT_STATE state;

    if (DWORD err = XInputGetState(dwIndex, &state); err != ERROR_SUCCESS) {
        reportNotConnected([&] { gInputLog.warn("XInputGetState({}) = {}", dwIndex, system::win32String(err)); });
        return false;
    }
    
    reportNotConnected.reset();

    const auto& [buttons, leftTrigger, rightTrigger, thumbLX, thumbLY, thumbRX, thumbRY] = state.Gamepad;

    if (stickInsideDeadzone(thumbLX, thumbLY, kLeftDeadzone)) {
        result.axis[Axis::padLeftStickVertical] = 0.f;
        result.axis[Axis::padLeftStickHorizontal] = 0.f;
    } else {
        dirty = true;
        result.axis[Axis::padLeftStickVertical] = float(thumbLY) / SHRT_MAX;
        result.axis[Axis::padLeftStickHorizontal] = float(thumbLX) / SHRT_MAX;
    }

    if (stickInsideDeadzone(thumbRX, thumbRY, kRightDeadzone)) {
        result.axis[Axis::padRightStickVertical] = 0.f;
        result.axis[Axis::padRightStickHorizontal] = 0.f;
    } else {
        dirty = true;
        result.axis[Axis::padRightStickVertical] = float(thumbRY) / SHRT_MAX;
        result.axis[Axis::padRightStickHorizontal] = float(thumbRX) / SHRT_MAX;
    }

    /// triggers
    result.axis[Axis::padLeftTrigger] = triggerRatio(leftTrigger, kTriggerDeadzone, dirty);
    result.axis[Axis::padRightTrigger] = triggerRatio(rightTrigger, kTriggerDeadzone, dirty);

    for (const auto& [slot, mask] : kGamepadKeys) {
        dirty |= updateKey(result, slot, mask, buttons);
    }

    return dirty;
}

bool Gamepad::updateKey(State& result, Key key, WORD mask, WORD state) {
    bool pressed = state & mask;
    if (!pressed) {
        result.key[key] = 0;
        return false;
    }

    if (result.key[key]) { return true; }

    return detail::update(result.key[key], index++);
}
