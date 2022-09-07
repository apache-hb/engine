#include "root.h"

namespace engine::render {
    RootSignature::RootSignature(ID3D12Device* device, Com<ID3DBlob> blob) {
        check(device->CreateRootSignature(0, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(addr())), "failed to create root signature");
    }
    
    namespace root {
        Com<ID3DBlob> compile(D3D_ROOT_SIGNATURE_VERSION version, const Create& create) {
            auto params = create.params;
            auto samplers = create.samplers;
            
            CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC desc;
            desc.Init_1_1(UINT(params.size()), params.data(), UINT(samplers.size()), samplers.data(), create.flags);
        
            Com<ID3DBlob> signature;
            Com<ID3DBlob> error;

            HRESULT hr = D3DX12SerializeVersionedRootSignature(&desc, version, &signature, &error);
            if (FAILED(hr)) {
                auto msg = error.valid() ? (const char*)error->GetBufferPointer() : "no error message";
                throw render::Error(hr, std::format("failed to serialize root signature\n{}", msg));
            }

            return signature;
        }
    }
}
