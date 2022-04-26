#include "log.h"
#include "debug.h"

#include <windows.h>

#include <map>
#include <vector>
#include <mutex>

#include "util/strings.h"

#include "imgui/imgui.h"

#define RESET "\x1b[0m"
#define BLACK "\x1b[0;30m"
#define RED "\x1b[0;31m"
#define GREEN "\x1b[0;32m"
#define YELLOW "\x1b[0;33m"
#define BLUE "\x1b[0;34m"
#define MAGENTA "\x1b[0;35m"
#define CYAN "\x1b[0;36m"
#define WHITE "\x1b[0;37m"

using namespace engine;

namespace {
    static const char *kLevelStrings[engine::log::TOTAL] = {
        "info",
        "warn",
        "fatal"
    };
    
    static const char *kColouredLevelStrings[engine::log::TOTAL] = {
        GREEN "info" RESET,
        YELLOW "warn" RESET,
        RED "fatal" RESET
    };


    struct LogDebugObject : debug::DebugObject {
        using Super = debug::DebugObject;
        LogDebugObject() : Super("log") { }

        virtual void info() override {
            std::lock_guard guard(lock);

            if (ImGui::BeginTabBar("channels")) {
                for (const auto& [channel, entries] : channels) {
                    if (ImGui::BeginTabItem(channel.data())) {
                        for (const auto& [level, message] : entries) {
                            ImGui::Text("[%s] %s", kLevelStrings[level], message.c_str());
                        }
                        ImGui::EndTabItem();
                    }
                }

                ImGui::EndTabBar();
            }
        }

        using Entry = std::pair<log::Level, std::string>;

        std::mutex lock;
        std::map<std::string_view, std::vector<Entry>> channels;
    };

    auto* kDebug = new LogDebugObject();
}

namespace engine::log {
    void Channel::log(Level report, std::string_view message) {
        if (report < level) { return; }
        
        if (!sanitize) {
            sendData(report, message);
        } else {
            for (auto part : strings::split(message, "\n")) {
                sendData(report, part);
            }
        }
    }

    void Channel::sendData(Level report, std::string_view message) {
        send(report, message);

        std::lock_guard guard(kDebug->lock);
        kDebug->channels[name].emplace_back(report, message);
    }
        
    void ConsoleChannel::send(Level report, std::string_view message) {
        auto view = kColouredLevelStrings[report];
        const std::string full = std::format("{}[{}]: {}", channel(), view, message);
        fprintf(file, "%s\n", full.c_str());
    }

    DWORD ConsoleChannel::init() {
        DWORD mode = 0;
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        if (hConsole == nullptr) { return GetLastError(); }

        if (GetConsoleMode(hConsole, &mode) == 0) { return GetLastError(); }
        mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        if (SetConsoleMode(hConsole, mode) == 0) { return GetLastError(); }

        return 0;
    }

    Channel* global = new ConsoleChannel("global", stdout);
    Channel* loader = new ConsoleChannel("loader", stderr);
    Channel* render = new ConsoleChannel("render", stdout);
    Channel* discord = new ConsoleChannel("discord", stderr);
}
