#include "engine/input/input.h"

#include "engine/locale/locale.h"

using namespace simcoe;
using namespace simcoe::input;

namespace {
    std::string_view id(Device device) {
        switch (device) {
#define DEVICE(id, name) case Device::id: return name;
#include "engine/input/input.inl"
        default: return "input.device.unknown";
        }
    }

    std::string_view id(Key::Slot key) {
        switch (key) {
#define KEY(id, name) case Key::id: return name;
#include "engine/input/input.inl"
        default: return "input.key.unknown";
        }
    }

    std::string_view id(Axis::Slot axis) {
        switch (axis) {
#define AXIS(id, name) case Axis::id: return name;
#include "engine/input/input.inl"
        default: return "input.axis.unknown";
        }
    }
}

void Manager::poll() {
    bool update = false;
    State state = prevState;
    for (auto *pSource : sources) {
        if (pSource->poll(&state)) {
            state.source = pSource->kind;
            update = true;
        }
    }

    // nothing to update
    if (!update) { return; }

    for (auto *pListener : listeners) {
        pListener->update(state);
    }

    state = prevState;
}

std::string_view locale::get(Device device) {
    return locale::get(id(device));
}

std::string_view locale::get(Key::Slot key) {
    return locale::get(id(key));
}

std::string_view locale::get(Axis::Slot axis) {
    return locale::get(id(axis));
}
