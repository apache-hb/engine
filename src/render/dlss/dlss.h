#pragma once

#include "render/util.h"

namespace engine::render::ndk {
    struct DlssState {
        void create(ID3D12Device* device, std::wstring_view logs = L"dlss.log");
        void destroy();
    };
}
