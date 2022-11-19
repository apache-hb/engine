#pragma once

#include "engine/base/container/resource.h"
#include "engine/base/win32.h"

namespace simcoe {
    struct HandleDeleter {
        void operator()(HANDLE handle) const {
            CloseHandle(handle);
        }
    };

    using UniqueHandle = UniqueResource<HANDLE, INVALID_HANDLE_VALUE, HandleDeleter>;
}
