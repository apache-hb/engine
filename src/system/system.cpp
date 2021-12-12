#include "system.h"

#include <psapi.h>

namespace engine::system {
    win32::Result<Stats> getStats() {
        std::string name(MAX_COMPUTERNAME_LENGTH + 1, ' ');
        DWORD size = MAX_COMPUTERNAME_LENGTH + 1;

        if (GetComputerNameA(name.data(), &size)) {
            name.resize(size);
            return pass(Stats{ name });
        }

        return fail(GetLastError());
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
            GetModuleFileNameA(modules[i], name, MAX_PATH);
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

    void init() {
        SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    }
}
