#include "signature.h"

using namespace engine::render;

Com<ID3DBlob> engine::render::compileRootSignature(D3D_ROOT_SIGNATURE_VERSION version, const RootCreate& info) {
    auto params = info.params;
    auto samplers = info.samplers;

    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC desc;
    desc.Init_1_1(UINT(params.size()), params.data(), UINT(samplers.size()), samplers.data(), info.flags);

    Com<ID3DBlob> signature;
    Com<ID3DBlob> error;

    HRESULT hr = D3DX12SerializeVersionedRootSignature(&desc, version, &signature, &error);
    if (FAILED(hr)) {
        auto msg = error.valid() ? (const char*)error->GetBufferPointer() : "no error message";
        throw Error(hr, std::format("failed to serialize root signature\n{}", msg));
    }

    return signature;
}
