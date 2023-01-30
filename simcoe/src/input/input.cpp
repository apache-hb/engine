#include "simcoe/input/input.h"

using namespace simcoe;
using namespace simcoe::input;

void Manager::poll() {
    bool bPushUpdate = false;
    State state = last;
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

    last = state;
}
