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

    XINPUT_STATE Device::poll() {
        XINPUT_STATE state;
        ok = XInputGetState(index, &state) == ERROR_SUCCESS;
        return state;
    }
}
