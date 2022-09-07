#include "render/objects/library.h"

constexpr GUID sm6Id = { /* 76f5573e-f13a-40f5-b297-81ce9e18933f */
    0x76f5573e,
    0xf13a,
    0x40f5,
    { 0xb2, 0x97, 0x81, 0xce, 0x9e, 0x18, 0x93, 0x3f }
};

namespace engine::render {
    bool enableSm6() {
        HRESULT hr = D3D12EnableExperimentalFeatures(1, &sm6Id, nullptr, nullptr);
        return !FAILED(hr);
    }
}
