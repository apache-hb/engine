#include "simcoe/locale/locale.h"

using namespace simcoe;

Locale::Locale() { }
Locale::Locale(const KeyMap& map) : strings(map) { }

const char *Locale::get(logging::Level it) const {
    switch (it) {
#define LEVEL(id, name) case logging::id: return get(name);
#include "simcoe/core/logging.inc"
    default: return "unknown";
    }
}

const char *Locale::get(input::Device it) const {
    switch (it) {
#define DEVICE(id, name) case input::Device::id: return get(name);
#include "simcoe/input/input.inc"
    default: return "unknown";
    }
}

const char *Locale::get(input::Key it) const {
    switch (it) {
#define KEY(id, name) case input::Key::id: return get(name);
#include "simcoe/input/input.inc"
    default: return "unknown";
    }
}

const char *Locale::get(input::Axis it) const {
    switch (it) {
#define AXIS(id, name) case input::Axis::id: return get(name);
#include "simcoe/input/input.inc"
    default: return "unknown";
    }
}

const char *Locale::get(const char *key) const {
    return key;
}
