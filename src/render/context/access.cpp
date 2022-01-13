#include "render/context.h"

#include "render/objects/factory.h"

using namespace engine::render;

Factory& Context::getFactory() {
    return *info.factory;
}

Adapter& Context::getAdapter() {
    return getFactory().getAdapter(info.adapter);
}
