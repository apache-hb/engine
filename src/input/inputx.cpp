#include "inputx.h"

namespace engine::xinput {
    bool Device::valid() const {
        return ok;
    }

    void Device::vibrate(USHORT left, USHORT right) {
        XINPUT_VIBRATION buzz = {
            .wLeftMotorSpeed = left,
            .wRightMotorSpeed = right
        };
        ok = XInputSetState(index, &buzz) == ERROR_SUCCESS;
    }

    float ratioGivenDeadzone(int16_t value, int16_t deadzone, float total) {
        if (deadzone > value && value > -deadzone) {
            return 0.f;
        }

        return float(value) / total;
    }

    constexpr float stickMax = 32767.f;
    constexpr float triggerMax = 255.f;

    float stickRatio(int16_t value, int16_t deadzone) {
        return ratioGivenDeadzone(value, deadzone, stickMax);
    }

    float triggerRatio(int16_t value, int16_t deadzone) {
        return ratioGivenDeadzone(value, deadzone, triggerMax);
    }

    Controller Device::poll() {
        XINPUT_STATE state;
        ok = XInputGetState(index, &state) == ERROR_SUCCESS;

        Controller controller;
        controller.index = state.dwPacketNumber;

        controller.lstick.x = stickRatio(state.Gamepad.sThumbLX, lstick.x);
        controller.lstick.y = stickRatio(state.Gamepad.sThumbLY, lstick.y);

        controller.rstick.x = stickRatio(state.Gamepad.sThumbRX, rstick.x);
        controller.rstick.y = stickRatio(state.Gamepad.sThumbRY, rstick.y);

        controller.ltrigger = triggerRatio(state.Gamepad.bLeftTrigger, ltrigger);
        controller.rtrigger = triggerRatio(state.Gamepad.bRightTrigger, rtrigger);

        return controller;
    }
}
