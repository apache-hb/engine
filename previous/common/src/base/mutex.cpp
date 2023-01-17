#include "engine/base/mutex.h"

using namespace simcoe;
using namespace simcoe::threads;

Mutex::Mutex()
    : event(CreateMutexA(nullptr, false, nullptr))
{ }

void Mutex::wait() {
    WaitForSingleObject(event.get(), INFINITE);
}

void Mutex::signal() {
    SignalObjectAndWait(event.get(), event.get(), 0, false);
}
