#include "simcoe/input/raw.h"
#include "simcoe/core/system.h"
#include "simcoe/simcoe.h"

#include <unordered_map>
#include <hidusage.h>

using namespace simcoe;
using namespace simcoe::input;

namespace {
    constexpr Key kMouseKeys[] = { 
        Key::keyLeftMouse, 
        Key::keyRightMouse, 
        Key::keyMiddleMouse, 
        Key::keyMouseButton1, 
        Key::keyMouseButton2 
    };

    const std::unordered_map<WPARAM, const char*> kDeviceChange = {
        { GIDC_ARRIVAL, "added" },
        { GIDC_REMOVAL, "removed" },
    };

    std::string mouseInfo(const RID_DEVICE_INFO_MOUSE& info) {
        const auto& [id, sample, buttons, hwheel] = info;
        return std::format("mouse(id = {}, sample = {}/s, buttons = {}, hwheel = {})", id, sample, buttons, hwheel ? "present" : "absent");
    }

    std::string keyboardInfo(const RID_DEVICE_INFO_KEYBOARD& info) {
        const auto& [type, subtype, keyboardMode, numberOfFunctionKeys, numberOfIndicators, numberOfKeysTotal] = info;
        return std::format("keyboard(type = {}:{}:{}, keys = {} + {}, leds = {})",
            type, subtype, keyboardMode, numberOfKeysTotal, numberOfFunctionKeys, numberOfIndicators);
    }

    std::string hidInfo(const RID_DEVICE_INFO_HID& info) {
        const auto& [vendor, product, version, page, usage] = info;
        return std::format("hid(id = {}:{}, version = {}, usage = {}:{})", vendor, product, version, page, usage);
    }

    std::string deviceInfo(HANDLE hDevice) {
        RID_DEVICE_INFO info = { .cbSize = sizeof(RID_DEVICE_INFO) };
        UINT size = sizeof(RID_DEVICE_INFO);

        UINT err = GetRawInputDeviceInfo(hDevice, RIDI_DEVICEINFO, &info, &size);
        if (err == UINT_MAX) { 
            gInputLog.warn("GetRawInputDeviceInfo() = {}", system::win32LastErrorString());
            return "unknown"; 
        }

        switch (info.dwType) {
        case RIM_TYPEMOUSE: return mouseInfo(info.mouse);
        case RIM_TYPEKEYBOARD: return keyboardInfo(info.keyboard);
        case RIM_TYPEHID: return hidInfo(info.hid);
        default: return std::format("unknown(type = {})", info.dwType);
        }
    }
}

RawDevicePool::RawDevicePool() : ISource(Device::eDesktop) {
    std::array<RAWINPUTDEVICE, 2> inputs;

    inputs[0] = {
        .usUsagePage = HID_USAGE_PAGE_GENERIC,
        .usUsage = HID_USAGE_GENERIC_MOUSE,
        .dwFlags = RIDEV_NOLEGACY
    };

    inputs[1] = {
        .usUsagePage = HID_USAGE_PAGE_GENERIC,
        .usUsage = HID_USAGE_GENERIC_KEYBOARD,
        .dwFlags = RIDEV_NOLEGACY
    };

    UINT err = RegisterRawInputDevices(inputs.data(), UINT(inputs.size()), sizeof(RAWINPUTDEVICE));
    if (err == FALSE) {
        gInputLog.fatal("RegisterRawInputDevices() = {}", err);
    }
}

bool RawDevicePool::poll(State& state) {
    bool changed = false;

    for (auto& type : devices) {
        for (auto& device : type) {
            changed |= device->poll(state);
        }
    }

    return changed;
}

void RawDevicePool::update(UINT msg, WPARAM wparam, LPARAM lparam) {
    if (msg == WM_INPUT_DEVICE_CHANGE) {
        gInputLog.warn("unhandled device change");
        gInputLog.warn("change type: {}", kDeviceChange.at(wparam));
        gInputLog.info("device info: {}", deviceInfo(HANDLE(lparam)));
        return;
    }

    if (msg != WM_INPUT) { return; }

    HRAWINPUT hInput = HRAWINPUT(lparam);
    UINT size = 0;
    UINT err = GetRawInputData(hInput, RID_INPUT, nullptr, &size, sizeof(RAWINPUTHEADER));
    if (err == UINT_MAX || size == 0) { 
        noInput([&] { gInputLog.warn("GetRawInputData() = {} (size = {})", system::win32LastErrorString(), size); });
        return; 
    }
    noInput.reset();

    // reserve space for the input
    buffer.resize(size);

    UINT expectedSize = size;
    err = GetRawInputData(hInput, RID_INPUT, buffer.data(), &expectedSize, sizeof(RAWINPUTHEADER));
    if (err == UINT_MAX || expectedSize != size) {
        dataSize([&] { gInputLog.warn("GetRawInputData() = {} (size = {}, expected = {})", system::win32LastErrorString(), size, expectedSize); });
        return;
    }
    dataSize.reset();

    PRAWINPUT pInput = PRAWINPUT(buffer.data());
    RAWINPUTHEADER header = pInput->header;

    if (header.dwType > RIM_TYPEMAX) {
        invalidType([&] { gInputLog.warn("Invalid raw input type: {}", header.dwType); });
        return;
    }
    invalidType.reset();

    onEvent(header.dwType, pInput);
}

void RawDevicePool::onEvent(DWORD type, PRAWINPUT pInput) {
    HANDLE hDevice = pInput->header.hDevice;

    if (!sendEvent(type, hDevice, pInput)) {
        gInputLog.info("new device: {}", deviceInfo(hDevice));
        addDevice(type, hDevice);
        sendEvent(type, hDevice, pInput);
    }
}

bool RawDevicePool::sendEvent(DWORD type, HANDLE hDevice, PRAWINPUT pInput) {
    for (auto& pDevice : devices[type]) {
        if (pDevice->getHandle() == hDevice) { 
            pDevice->update(pInput);
            return true;
        }
    }

    return false;
}

void RawDevicePool::addDevice(DWORD type, HANDLE hDevice) {
    switch (type) {
    case RIM_TYPEMOUSE: addMouse(hDevice); break;
    case RIM_TYPEKEYBOARD: addKeyboard(hDevice); break;
    default: gInputLog.warn("Unknown device type: {}", type); break;
    }
}

void RawDevicePool::addMouse(HANDLE hDevice) {
    IRawDevice *pDevice = new RawMouse(hDevice);
    devices[RIM_TYPEMOUSE].emplace(pDevice);
}

void RawDevicePool::addKeyboard(HANDLE hDevice) {
    IRawDevice *pDevice = new RawKeyboard(hDevice);
    devices[RIM_TYPEKEYBOARD].emplace(pDevice);
}

RawMouse::RawMouse(HANDLE hDevice) 
    : IRawDevice(Device::eDesktop, hDevice) 
{ }

bool RawMouse::poll(State& state) {
    bool changed = false;
    for (auto id : kMouseKeys) {
        if (keys[id] != 0) {
            state.key[id] = keys[id];
            changed = true;
        }
    }

    if (absolute != base) {
        changed = true;
        state.axis[Axis::mouseHorizontal] = absolute.x - base.x;
        state.axis[Axis::mouseVertical] = absolute.y - base.y;
    }

    return changed;
}

void RawMouse::update(PRAWINPUT pInput) {
    RAWINPUTHEADER header = pInput->header;
    ASSERT(header.dwType == RIM_TYPEMOUSE);

    RAWMOUSE mouse = pInput->data.mouse;

    switch (mouse.usButtonFlags) {
    case RI_MOUSE_LEFT_BUTTON_DOWN: keys[Key::keyLeftMouse] = index++; break;
    case RI_MOUSE_LEFT_BUTTON_UP: keys[Key::keyLeftMouse] = 0; break;
    case RI_MOUSE_RIGHT_BUTTON_DOWN: keys[Key::keyRightMouse] = index++; break;
    case RI_MOUSE_RIGHT_BUTTON_UP: keys[Key::keyRightMouse] = 0; break;
    case RI_MOUSE_MIDDLE_BUTTON_DOWN: keys[Key::keyMiddleMouse] = index++; break;
    case RI_MOUSE_MIDDLE_BUTTON_UP: keys[Key::keyMiddleMouse] = 0; break;
    case RI_MOUSE_BUTTON_4_DOWN: keys[Key::keyMouseButton1] = index++; break;
    case RI_MOUSE_BUTTON_4_UP: keys[Key::keyMouseButton1] = 0; break;
    case RI_MOUSE_BUTTON_5_DOWN: keys[Key::keyMouseButton2] = index++; break;
    case RI_MOUSE_BUTTON_5_UP: keys[Key::keyMouseButton2] = 0; break;
    default: break;
    }

    if (mouse.usFlags & MOUSE_MOVE_ABSOLUTE) {
        base = absolute;
        absolute += { float(mouse.lLastX), float(mouse.lLastY) };
    } else {
        base = absolute;
        absolute = { float(mouse.lLastX), float(mouse.lLastY) };
    }
}

RawKeyboard::RawKeyboard(HANDLE hDevice) 
    : IRawDevice(Device::eDesktop, hDevice) 
{ }

bool RawKeyboard::poll(State&) {
    return false;
}

void RawKeyboard::update(PRAWINPUT) {

}
