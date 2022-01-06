#include "inputx.h"

namespace engine::xinput {
    bool Device::valid() const noexcept {
        return ok;
    }

    void Device::vibrate(USHORT left, USHORT right) noexcept {
        XINPUT_VIBRATION buzz = {
            .wLeftMotorSpeed = left,
            .wRightMotorSpeed = right
        };
        ok = XInputSetState(index, &buzz) == ERROR_SUCCESS;
    }

    /**
     * @brief get a value between -1.0 and 1.0, given a deadzone value as a lower cutoff
     * 
     * @param value the 16 bit value to be normalized
     * @param deadzone the deadzone bounds
     * @param total the maximum value of the axis
     * @return the normalized value
     */
    constexpr float ratioGivenDeadzone(int16_t value, int16_t deadzone, float total) {
        if (deadzone > value && value > -deadzone) {
            return 0.f;
        }

        return float(value) / total;
    }

    constexpr auto kStickMax = 32767.f;
    constexpr auto kTriggerMax = 255.f;

    constexpr float stickRatio(int16_t value, int16_t deadzone) {
        return ratioGivenDeadzone(value, deadzone, kStickMax);
    }

    constexpr float triggerRatio(int16_t value, int16_t deadzone) {
        return ratioGivenDeadzone(value, deadzone, kTriggerMax);
    }

    input::Input Device::poll() noexcept {
        XINPUT_STATE state;
        ok = XInputGetState(index, &state) == ERROR_SUCCESS;

        input::Input controller;
        controller.lstick.x = stickRatio(state.Gamepad.sThumbLX, lstick.x);
        controller.lstick.y = stickRatio(state.Gamepad.sThumbLY, lstick.y);

        controller.rstick.x = stickRatio(state.Gamepad.sThumbRX, rstick.x);
        controller.rstick.y = stickRatio(state.Gamepad.sThumbRY, rstick.y);

        controller.ltrigger = triggerRatio(state.Gamepad.bLeftTrigger, ltrigger);
        controller.rtrigger = triggerRatio(state.Gamepad.bRightTrigger, rtrigger);

        return controller;
    }
}
