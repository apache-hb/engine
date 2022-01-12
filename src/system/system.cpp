#include "system.h"

#include "logging/log.h"

#include <psapi.h>

namespace engine::system {
    RECT rectCoords(LONG x, LONG y, LONG width, LONG height) {
        RECT rect = { x, y, x + width, y + height };
        return rect;
    }
    
    Stats getStats() {
        std::string name(MAX_COMPUTERNAME_LENGTH + 1, ' ');
        DWORD size = MAX_COMPUTERNAME_LENGTH + 1;

        if (!GetComputerNameA(name.data(), &size)) {
            throw win32::Error("GetComputerNameA");
        }

        name.resize(size);
        return Stats { name };
    }

    std::vector<std::string> loadedModules() {
        std::vector<HMODULE> modules;

        HANDLE self = GetCurrentProcess();
        DWORD elements = 0;

        EnumProcessModules(self, nullptr, 0, &elements);
        modules.resize(elements / sizeof(HMODULE));

        EnumProcessModules(self, modules.data(), elements, &elements);
    
        std::vector<std::string> paths(modules.size());

        for (size_t i = 0; i < modules.size(); i++) {
            char name[MAX_PATH];
            if (!GetModuleFileNameA(modules[i], name, MAX_PATH)) {
                throw win32::Error("GetModuleFileNameA");
            }
            paths[i] = name;
        }

        return paths;
    }

    std::vector<std::string> detrimentalModules(const std::vector<std::string>& all) {
        std::vector<std::string> result;
        
        for (auto mod : all) {
            if (mod.find("AsusWallpaperSettingHook.dll") != std::string::npos) {
                result.push_back(mod);
            }
        }

        return result;
    }

    std::vector<Display> displays() {
        std::vector<Display> result;

        DISPLAY_DEVICEA device = { .cb = sizeof(DISPLAY_DEVICEA) };

        DWORD index = 0;
        while (EnumDisplayDevicesA(nullptr, index, &device, 0)) {
            result.push_back(Display(device));
            index += 1;
        }

        return result;
    }

    Module::Module(std::wstring_view path) {
        handle = LoadLibraryW(path.data());
    }

    bool Module::found() const { return handle != nullptr; }

    void* Module::getAddr(std::string_view name) const {
        if (!found()) { return nullptr; }
        
        return GetProcAddress(handle, name.data());
    }

    void init() {
        if (!SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2)) {
            log::global->warn("SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2)");
        }
    }
}
