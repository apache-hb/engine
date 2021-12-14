#include "logs.h"

namespace engine::logs {
    logging::Channel *global = new logging::ConsoleChannel("global", stdout);
    logging::Channel *render = new logging::ConsoleChannel("render", stdout);
}
