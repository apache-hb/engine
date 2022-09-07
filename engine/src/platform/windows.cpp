#include "engine/platform/platform.h"

#include <windows.h>

using namespace engine::platform;

namespace {
    [[nodiscard]]
    std::string getComputerName() noexcept {
        constexpr auto kNameLength = MAX_COMPUTERNAME_LENGTH + 1;
        
        char name[kNameLength];
        DWORD size = kNameLength;

        if (!GetComputerNameA(name, &size)) {
            return "";
        }

        return std::string {name, size};
    }

    [[nodiscard]]
    std::vector<Display> getDisplayDevices() noexcept {
        std::vector<Display> displays;

        DISPLAY_DEVICEA display = { .cb = sizeof(DISPLAY_DEVICEA) };
        for (DWORD i = 0; EnumDisplayDevicesA(nullptr, i, &display, 0); i++) {
            
            /* we dont want displays we cant display anything on */
            bool active = display.StateFlags & DISPLAY_DEVICE_ACTIVE;
            if (!active) {
                continue;
            }

            std::string name = display.DeviceName;
            std::string desc = display.DeviceString;
            bool primary = display.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE;
            
            displays.push_back({ name, desc, primary });
        }

        return displays;
    }
}

System::System() noexcept
    : name(getComputerName())
    , displays(getDisplayDevices())
{ }
