#include "engine/base/logging.h"
#include "engine/input/input.h"
#include <winuser.h>

using namespace simcoe;
using namespace simcoe::input;

/// 
/// keyboard support via win32
///

namespace {
    POINT windowCenter(HWND hwnd) {
        RECT rect;
        GetWindowRect(hwnd, &rect);

        POINT center;
        center.x = (rect.left + rect.right) / 2;
        center.y = (rect.top + rect.bottom) / 2;

        return center;
    }

    POINT cursorPos() {
        POINT pos;
        GetCursorPos(&pos);
        return pos;
    }

    struct DesktopKeyMapping {
        Key::Slot slot;
        int vk;
    };

    constexpr DesktopKeyMapping kDesktopKeys[] = {
        { Key::keyA, 'A' },
        { Key::keyB, 'B' },
        { Key::keyC, 'C' },
        { Key::keyD, 'D' },
        { Key::keyE, 'E' },
        { Key::keyF, 'F' },
        { Key::keyG, 'G' },
        { Key::keyH, 'H' },
        { Key::keyI, 'I' },
        { Key::keyJ, 'J' },
        { Key::keyK, 'K' },
        { Key::keyL, 'L' },
        { Key::keyM, 'M' },
        { Key::keyN, 'N' },
        { Key::keyO, 'O' },
        { Key::keyP, 'P' },
        { Key::keyQ, 'Q' },
        { Key::keyR, 'R' },
        { Key::keyS, 'S' },
        { Key::keyT, 'T' },
        { Key::keyU, 'U' },
        { Key::keyV, 'V' },
        { Key::keyW, 'W' },
        { Key::keyX, 'X' },
        { Key::keyY, 'Y' },
        { Key::keyZ, 'Z' },

        { Key::key0, '0' },
        { Key::key1, '1' },
        { Key::key2, '2' },
        { Key::key3, '3' },
        { Key::key4, '4' },
        { Key::key5, '5' },
        { Key::key6, '6' },
        { Key::key7, '7' },
        { Key::key8, '8' },
        { Key::key9, '9' },

        { Key::keyLeft, VK_LEFT },
        { Key::keyRight, VK_RIGHT },
        { Key::keyUp, VK_UP },
        { Key::keyDown, VK_DOWN },

        { Key::keyEsc, VK_ESCAPE },
        { Key::keyLeftShift, VK_LSHIFT },
        { Key::keyRightShift, VK_RSHIFT },
        { Key::keyLeftControl, VK_LCONTROL },
        { Key::keyRightControl, VK_RCONTROL },
        { Key::keyLeftAlt, VK_LMENU },
        { Key::keyRightAlt, VK_RMENU },
        { Key::keySpace, VK_SPACE },

        { Key::keyTilde, VK_OEM_3 },

        { Key::keyLeftMouse, VK_LBUTTON },
        { Key::keyRightMouse, VK_RBUTTON },
        { Key::keyMiddleMouse, VK_MBUTTON },
    };
}

Keyboard::Keyboard(bool capture) : Source(Device::eDesktop), enabled(true) {
    captureInput(capture);
}

bool Keyboard::poll(State *pState) {
    for (const auto& key : kDesktopKeys) {
        pState->key[key.slot] = keys[key.vk];
    }

    if (!enabled) { return true; }

    pState->axis[Axis::mouseHorizontal] = cursor.x;
    pState->axis[Axis::mouseVertical] = cursor.y;

    return true;
}

void Keyboard::captureInput(bool capture) {
    if (enabled == capture) { return; }

    enabled = capture;
    win32::showCursor(!enabled);
}

void Keyboard::update(HWND hwnd, const KeyState newKeys) {
    if (enabled) {
        auto [x, y] = cursorPos();
        auto [cx, cy] = windowCenter(hwnd);
        SetCursorPos(cx, cy);
        cursor = { float(x - cx), float(cy - y) };
    } else {
        cursor = { 0, 0 };
    }

    memcpy(keys, newKeys, sizeof(KeyState));
}
