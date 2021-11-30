#include "log.h"

#include <windows.h>

#define RESET "\x1b[0m"
#define BLACK "\x1b[0;30m"
#define RED "\x1b[0;31m"
#define GREEN "\x1b[0;32m"
#define YELLOW "\x1b[0;33m"
#define BLUE "\x1b[0;34m"
#define MAGENTA "\x1b[0;35m"
#define CYAN "\x1b[0;36m"
#define WHITE "\x1b[0;37m"

namespace engine::logging {
    void Channel::log(Level report, std::string_view message) {
        if (report < level) { return; }
        send(report, message);    
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

    void ConsoleChannel::init() {
        DWORD mode = 0;
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

        GetConsoleMode(hConsole, &mode);
        mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode(hConsole, mode);
    }
}
