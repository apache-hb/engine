#include "simcoe/input/desktop.h"
#include "simcoe/core/system.h"
#include "simcoe/core/util.h"

#include "simcoe/simcoe.h"

#include "common.h"

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

    math::float2 getCursorPoint() {
        POINT pos;
        GetCursorPos(&pos);
        return { float(pos.x), float(pos.y) };
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

        { VK_XBUTTON1, Key::keyMouseButton1 },
        { VK_XBUTTON2, Key::keyMouseButton2 }
    };
}

Keyboard::Keyboard()
    : ISource(Device::eDesktop)
{ }

bool Keyboard::poll(State& result) {
    bool dirty = false;
    for (const auto& [vk, slot] : kDesktopKeys) {
        dirty |= detail::update(result.key[slot], keys[slot]);
    }

    return dirty;
}

void Keyboard::update(UINT msg, WPARAM wparam, LPARAM lparam) {
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

    case WM_XBUTTONDOWN:
        setXButton(GET_XBUTTON_WPARAM(wparam), index++);
        break;

    case WM_XBUTTONUP:
        setXButton(GET_XBUTTON_WPARAM(wparam), 0);
        break;

    default:
        break;
    }
}

namespace {
    std::unordered_map<WORD, util::DoOnce> unknownButton;
}

void Keyboard::setKey(WORD key, size_t value) {
    auto it = kDesktopKeys.find(key);
    if (it == kDesktopKeys.end()) {
        unknownButton[key]([&] { gInputLog.warn("Unknown key: {}", key); });
        return;
    }

    keys[it->second] = value;
}

void Keyboard::setXButton(WORD key, size_t value) {
    switch (key) {
    case XBUTTON1:
        setKey(VK_XBUTTON1, value);
        break;
    case XBUTTON2:
        setKey(VK_XBUTTON2, value);
        break;
    default:
        unknownButton[key]([&] { gInputLog.warn("Unknown XButton: {}", key); });
        setKey(key, value);
        break;
    }
}

Mouse::Mouse(bool lockCursor, bool readCursor)
    : ISource(Device::eDesktop)
    , captured(lockCursor)
    , enabled(readCursor)
{ }

bool Mouse::poll(State& result) {
    result.axis[Axis::mouseHorizontal] = absolute.x - base.x;
    result.axis[Axis::mouseVertical] = absolute.y - base.y;

    return base != absolute;
}

void Mouse::captureInput(bool capture) {
    captured = capture;
}

void Mouse::update(HWND hWnd) {
    if (!enabled) {
        absolute = base;
        return;
    }

    if (captured) {
        absolute = getCursorPoint();
        auto [cx, cy] = getWindowCenter(hWnd);
        SetCursorPos(cx, cy);

        // set the base to the center of the window
        base = { float(cx), float(cy) };
    } else {
        // set the base to the last absolute position
        base = absolute;
        absolute = getCursorPoint();
    }
}
