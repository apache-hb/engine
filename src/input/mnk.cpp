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

        result.rstick.x = float(newX - mouseX) * sensitivity;
        result.rstick.y = -float(newY - mouseY) * sensitivity;

        result.lstick.x = -float(keys['A'] - keys['D']);
        result.lstick.y = -float(keys['S'] - keys['W']);

        result.ltrigger = float(keys['Q']);
        result.rtrigger = float(keys['E']);

        mouseX = newX;
        mouseY = newY;

        return result;
    }
}
