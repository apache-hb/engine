#include "render/render.h"

namespace engine::render {
    constexpr auto kDefaultProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    constexpr auto kUploadProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

    constexpr D3D12_SUBRESOURCE_DATA createUploadDesc(const void *data, size_t size) {
        return { 
            .pData = data,
            .RowPitch = LONG_PTR(size),
            .SlicePitch = LONG_PTR(size)
        };
    }

    constexpr D3D12_RESOURCE_DESC createResourceDesc(UINT width, UINT height, DXGI_FORMAT format) {
        return {
            .Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
            .Width = width,
            .Height = height,
            .DepthOrArraySize = 1,
            .MipLevels = 1,
            .Format = format,
            .SampleDesc = { 1, 0 }
        };
    }

    constexpr D3D12_SHADER_RESOURCE_VIEW_DESC createSRVDesc(DXGI_FORMAT format) {
        return {
            .Format = format,
            .ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D,
            .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
            .Texture2D = { .MipLevels = 1 }
        };
    }

    Resource Context::uploadData(std::wstring_view name, const void* data, UINT size) {
        log::render->info("uploading vertex buffer {}", strings::narrow(name));

        const auto bufferSize = CD3DX12_RESOURCE_DESC::Buffer(size);
        const auto uploadDesc = createUploadDesc(data, size);

        Resource defaultResource;
        Resource uploadResource;
        
        HRESULT hrDefault = device->CreateCommittedResource(
            &kDefaultProps, D3D12_HEAP_FLAG_NONE,
            &bufferSize, D3D12_RESOURCE_STATE_COMMON,
            nullptr, IID_PPV_ARGS(&defaultResource)
        );
        check(hrDefault, "failed to create default resource");

        HRESULT hrUpload = device->CreateCommittedResource(
            &kUploadProps, D3D12_HEAP_FLAG_NONE,
            &bufferSize, D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr, IID_PPV_ARGS(&uploadResource)
        );
        check(hrUpload, "failed to create upload resource");

        defaultResource.rename(name.data());
        uploadResource.rename(std::format(L"upload-resource-{}", name).data());

        auto ptr = uploadResource.map(0);
        memcpy(ptr, data, size);
        uploadResource.unmap(0);

        copyCommands->CopyBufferRegion(defaultResource.get(), 0, uploadResource.get(), 0, size);
    
        copyResources.push_back(uploadResource);

        return defaultResource;
    }

    VertexBuffer Context::uploadVertexData(std::wstring_view name, const void* data, UINT size, UINT stride) {
        auto resource = uploadData(name, data, size);
        return VertexBuffer(resource, {
            .BufferLocation = resource.gpuAddress(),
            .SizeInBytes = size,
            .StrideInBytes = stride
        });
    }
}
