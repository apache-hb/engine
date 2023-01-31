#include "simcoe/input/desktop.h"
#include "simcoe/simcoe.h"

#include <unordered_map>

using namespace simcoe;
using namespace simcoe::input;

namespace {
    POINT getWindowCenter(HWND hwnd) {
        RECT rect;
        GetWindowRect(hwnd, &rect);

        POINT center;
        center.x = (rect.left + rect.right) / 2;
        center.y = (rect.top + rect.bottom) / 2;

        return center;
    }

    POINT getCursorPoint() {
        POINT pos;
        GetCursorPos(&pos);
        return pos;
    }

    // handling keyboard accuratley is more tricky than it first seems
    // https://learn.microsoft.com/en-us/windows/win32/inputdev/about-keyboard-input#keystroke-message-flags
    WORD mapKeyCode(WPARAM wparam, LPARAM lparam) {
        WORD vkCode = LOWORD(wparam);
        WORD keyFlags = HIWORD(lparam);
        WORD scanCode = LOBYTE(keyFlags);

        if ((keyFlags & KF_EXTENDED) == KF_EXTENDED) {
            scanCode = MAKEWORD(scanCode, 0xE0);
        }

        if (vkCode == VK_SHIFT || vkCode == VK_CONTROL || vkCode == VK_MENU) {
            vkCode = LOWORD(MapVirtualKeyA(scanCode, MAPVK_VSC_TO_VK_EX));
        }

        return vkCode;
    }

    const std::unordered_map<int, Key> kDesktopKeys = {
        { 'A', Key::keyA },
        { 'B', Key::keyB },
        { 'C', Key::keyC },
        { 'D', Key::keyD },
        { 'E', Key::keyE },
        { 'F', Key::keyF },
        { 'G', Key::keyG },
        { 'H', Key::keyH },
        { 'I', Key::keyI },
        { 'J', Key::keyJ },
        { 'K', Key::keyK },
        { 'L', Key::keyL },
        { 'M', Key::keyM },
        { 'N', Key::keyN },
        { 'O', Key::keyO },
        { 'P', Key::keyP },
        { 'Q', Key::keyQ },
        { 'R', Key::keyR },
        { 'S', Key::keyS },
        { 'T', Key::keyT },
        { 'U', Key::keyU },
        { 'V', Key::keyV },
        { 'W', Key::keyW },
        { 'X', Key::keyX },
        { 'Y', Key::keyY },
        { 'Z', Key::keyZ },

        { '0', Key::key0 },
        { '1', Key::key1 },
        { '2', Key::key2 },
        { '3', Key::key3 },
        { '4', Key::key4 },
        { '5', Key::key5 },
        { '6', Key::key6 },
        { '7', Key::key7 },
        { '8', Key::key8 },
        { '9', Key::key9 },

        { VK_LEFT, Key::keyLeft },
        { VK_RIGHT, Key::keyRight },
        { VK_UP, Key::keyUp },
        { VK_DOWN, Key::keyDown },

        { VK_ESCAPE, Key::keyEsc },
        { VK_LSHIFT, Key::keyLeftShift },
        
        { VK_RSHIFT, Key::keyRightShift },
        { VK_LCONTROL, Key::keyLeftControl },
        { VK_RCONTROL, Key::keyRightControl },
        { VK_LMENU, Key::keyLeftAlt },
        { VK_RMENU, Key::keyRightAlt },
        { VK_SPACE, Key::keySpace },

        { VK_RETURN, Key::keyEnter },

        { VK_OEM_3, Key::keyTilde },

        { VK_LBUTTON, Key::keyLeftMouse },
        { VK_RBUTTON, Key::keyRightMouse },
        { VK_MBUTTON, Key::keyMiddleMouse },
    };
}

Desktop::Desktop(bool capture)
    : ISource(Device::eDesktop)
{ 
    captureInput(capture);
}

bool Desktop::poll(State& result) {
    std::copy(keys.begin(), keys.end(), result.key.begin());

    if (!enabled) {
        return true;
    }

    result.axis[Axis::mouseHorizontal] = mouse.x;
    result.axis[Axis::mouseVertical] = mouse.y;

    return true;
}

void Desktop::captureInput(bool capture) {
    enabled = capture;
    system::showCursor(!capture);
}

void Desktop::updateKeys(UINT msg, WPARAM wparam, LPARAM lparam) {
    switch (msg) {
    case WM_SYSKEYDOWN:
    case WM_KEYDOWN:
        // ignore repeated keys
        if (HIWORD(lparam) & KF_REPEAT) {
            return;
        }

        setKey(mapKeyCode(wparam, lparam), index++);
        break;
    case WM_SYSKEYUP:
    case WM_KEYUP:
        setKey(mapKeyCode(wparam, lparam), 0);
        break;

    case WM_RBUTTONDOWN:
        setKey(VK_RBUTTON, index++);
        break;
    case WM_RBUTTONUP:
        setKey(VK_RBUTTON, 0);
        break;

    case WM_LBUTTONDOWN:
        setKey(VK_LBUTTON, index++);
        break;
    case WM_LBUTTONUP:
        setKey(VK_LBUTTON, 0);
        break;

    case WM_MBUTTONDOWN:
        setKey(VK_MBUTTON, index++);
        break;
    case WM_MBUTTONUP:
        setKey(VK_MBUTTON, 0);
        break;
    }
}

void Desktop::updateMouse(HWND hWnd) {
    if (!enabled) {
        mouse = { 0, 0 };
        return;
    }

    auto [x, y] = getCursorPoint();
    auto [cx, cy] = getWindowCenter(hWnd);

    SetCursorPos(cx, cy);
    mouse = { float(x - cx), float(y - cy) };
}

void Desktop::setKey(WORD key, size_t value) {
    auto it = kDesktopKeys.find(key);
    if (it == kDesktopKeys.end()) {
        gInputLog.warn("Unknown key: {}", key);
        return;
    }
    keys[it->second] = value;
}
