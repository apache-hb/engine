#include "simcoe/input/input.h"

using namespace simcoe;
using namespace simcoe::input;

ISource::ISource(Device kind) : kind(kind) { }

void Manager::poll() {
    bool bPushUpdate = false;
    for (ISource *pSource : sources) {
        if (pSource->poll(state)) {
            state.device = pSource->kind;
            bPushUpdate = true;
        }
    }

    if (!bPushUpdate) { return; }

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
