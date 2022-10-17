#include "engine/base/logging.h"
#include "engine/input/input.h"

using namespace simcoe;
using namespace simcoe::input;

/// 
/// keyboard support via win32
///

namespace {
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
    };
}

Keyboard::Keyboard() 
    : Source(Device::eDesktop) 
{ }

bool Keyboard::poll(UNUSED State *pState) {
    return false;
}

void Keyboard::update() {

}