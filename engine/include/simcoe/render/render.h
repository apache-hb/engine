#pragma once

#include "simcoe/core/panic.h"
#include "simcoe/core/system.h"
#include <dx/d3dx12/d3dx12.h>

#define RELEASE(p) do { if (p != nullptr) { p->Release(); p = nullptr; } } while (0)
