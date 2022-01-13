#include "render/context.h"

using namespace engine::render;

Context::Context(Create&& create)
    : info(create)
    , events(info.budget.queueSize)
{ }

void Context::create() {
    createDevice();
    createViews();
}

void Context::destroy() {
    destroyDevice();
}

void Context::present() {

}

void Context::addEvent(Event&& event) {
    events.push(std::move(event));
}
