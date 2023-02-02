#include "simcoe/input/input.h"

using namespace simcoe;
using namespace simcoe::input;

ISource::ISource(Device kind) : kind(kind) { }

void Manager::poll() {
    bool dirty = false;
    for (ISource *pSource : sources) {
        if (pSource->poll(state)) {
            dirty = true;
            state.device = pSource->kind;
        }
    }

    if (!dirty) { return; }

    for (ITarget *pTarget : targets) {
        pTarget->accept(state);
    }
}

void Manager::add(ISource *pSource) {
    sources.push_back(pSource);
}

void Manager::add(ITarget *pTarget) {
    targets.push_back(pTarget);
}
