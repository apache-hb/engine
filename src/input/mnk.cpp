#include "mnk.h"
#include "logging/log.h"
#include <windowsx.h>

namespace engine::input {
    void MnK::pushPress(WPARAM wparam) {
        keys[wparam] = true;
    }

    void MnK::pushRelease(WPARAM wparam) {
        keys[wparam] = false;
    }

    Input MnK::poll() {
        Input result;

        POINT cursor;
        GetCursorPos(&cursor);
        newX = cursor.x;
        newY = cursor.y;

        float accel = (keys[VK_SHIFT] || keys[VK_LSHIFT] || keys[VK_RSHIFT] ? shiftAccel : 1.f);

        result.rstick.x = float(newX - mouseX) * sensitivity + float(keys[VK_RIGHT] - keys[VK_LEFT]) * sensitivity;
        result.rstick.y = -float(newY - mouseY) * sensitivity + float(keys[VK_UP] - keys[VK_DOWN]) * sensitivity;

        result.lstick.x = -float(keys['A'] - keys['D']) * accel;
        result.lstick.y = -float(keys['S'] - keys['W']) * accel;

        result.ltrigger = float(keys['Q']) * accel;
        result.rtrigger = float(keys['E']) * accel;

        mouseX = newX;
        mouseY = newY;

        return result;
    }
}
