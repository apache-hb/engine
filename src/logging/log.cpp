#include "log.h"

#include <windows.h>

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

namespace engine::log {
    void Channel::log(Level report, std::string_view message) {
        if (report < level) { return; }
        
        if (!sanitize) {
            send(report, message);
        } else {
            for (auto part : strings::split(message, "\n")) {
                send(report, part);
            }
        }
    }

    void Channel::infof(const char *message, ...) {
        va_list args;
        va_start(args, message);
        auto msg = strings::cformatv(message, args);
        va_end(args);
        log(INFO, msg);
    }

    void Channel::warnf(const char *message, ...) {
        va_list args;
        va_start(args, message);
        auto msg = strings::cformatv(message, args);
        va_end(args);
        log(WARN, msg);
    }

    void Channel::fatalf(const char * message, ...) {
        va_list args;
        va_start(args, message);
        auto msg = strings::cformatv(message, args);
        va_end(args);
        log(FATAL, msg);
    }

    namespace {
        std::string_view level_string(Level level) {
            switch (level) {
            case INFO: return GREEN "info" RESET;
            case WARN: return YELLOW "warn" RESET;
            case FATAL: return RED "fatal" RESET;
            default: return CYAN "error" RESET;
            }
        }
    }

    void ConsoleChannel::send(Level report, std::string_view message) {
        auto view = level_string(report);
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

    struct BroadcastChannel : Channel {
        using Channels = std::vector<Channel*>;
        BroadcastChannel(std::string_view name, Channels channels)
            : Channel(name, false)
            , channels(channels) 
        { }

        virtual void send(Level report, std::string_view message) override {
            for (auto channel : channels) {
                channel->send(report, message);
            }
        }

        virtual void tick() override {
            for (auto channel : channels) {
                channel->tick();
            }
        }
    private:
        Channels channels;
    };

    struct ImGuiChannel : Channel {
        using Report = std::tuple<Level, std::string>;

        ImGuiChannel(std::string_view name) 
            : Channel(name, false) 
        { }

        virtual void send(Level report, std::string_view message) override {
            reports.push_back(Report(report, std::string(message)));
        }

        virtual void tick() override {
            if (ImGui::Begin(channel().data())) {
                ImGui::Text("double click a log to copy it to clipboard");
                ImGui::Separator();
                ImGui::PushTextWrapPos(ImGui::GetWindowWidth() * 0.9f);

                for (const auto& [level, report] : reports) {
                    auto colour = reportColour(level);
                    ImGui::PushStyleColor(ImGuiCol_Text, colour);
                    if (ImGui::Selectable(report.c_str(), false, ImGuiSelectableFlags_AllowDoubleClick)) {
                        if (ImGui::IsMouseDoubleClicked(0)) {
                            ImGui::SetClipboardText(report.c_str());
                        }
                    }
                    ImGui::PopStyleColor();
                }
                
                ImGui::PopTextWrapPos();
            }
            ImGui::End();
        }
    private:
        static ImVec4 reportColour(Level level) {
            switch (level) {
            case INFO: return ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
            case WARN: return ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
            case FATAL: return ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
            default: return ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
            }
        }
        std::vector<Report> reports;
    };

    Channel *global = new ConsoleChannel("global", stdout);
    Channel *render = new BroadcastChannel("render", {
        new ConsoleChannel("render", stdout),
        new ImGuiChannel("console")
    });
}
