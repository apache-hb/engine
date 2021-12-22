#include "render.h"

#include "util/strings.h"

namespace engine::render {
    const auto defaultProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    const auto uploadProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

    constexpr D3D12_SUBRESOURCE_DATA createUploadDesc(const void *data, size_t size) {
        return { 
            .pData = data,
            .RowPitch = LONG_PTR(size),
            .SlicePitch = LONG_PTR(size)
        };
    }

    constexpr D3D12_SUBRESOURCE_DATA createTextureUploadDesc(const void *data, size_t width, size_t height, size_t bpp) {
        return { 
            .pData = data,
            .RowPitch = LONG_PTR(width * bpp),
            .SlicePitch = LONG_PTR(width * height * bpp)
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

    constexpr DXGI_FORMAT formatForBPP(size_t bpp) {
        switch (bpp) {
        case 3: return DXGI_FORMAT_R8G8B8A8_UNORM;
        case 4: return DXGI_FORMAT_R8G8B8A8_UNORM;
        default: return DXGI_FORMAT_UNKNOWN;
        }
    }
    
    Com<ID3D12Resource> Context::uploadBuffer(const void *data, size_t size, std::wstring_view name) {
        const auto bufferSize = CD3DX12_RESOURCE_DESC::Buffer(size);
        const auto uploadDesc = createUploadDesc(data, size);

        Com<ID3D12Resource> defaultResource;
        Com<ID3D12Resource> uploadResource;

        HRESULT hrDefault = device->CreateCommittedResource(
            &defaultProps, D3D12_HEAP_FLAG_NONE,
            &bufferSize, D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr, IID_PPV_ARGS(&defaultResource)
        );
        check(hrDefault, "failed to create default resource");

        HRESULT hrUpload = device->CreateCommittedResource(
            &uploadProps, D3D12_HEAP_FLAG_NONE,
            &bufferSize, D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr, IID_PPV_ARGS(&uploadResource)
        );
        check(hrUpload, "failed to create upload resource");

        defaultResource.rename(name.data());
        uploadResource.rename(std::format(L"upload-resource-{}", name).data());

        void *uploadBufferPtr;
        CD3DX12_RANGE readRange(0, 0);
        check(uploadResource->Map(0, &readRange, &uploadBufferPtr), "failed to map upload resource");
        memcpy(uploadBufferPtr, data, size);
        uploadResource->Unmap(0, nullptr);

        UpdateSubresources(copyCommandList.get(), defaultResource.get(), uploadResource.get(), 0, 0, 1, &uploadDesc);

        copyResources.push_back(uploadResource);

        return defaultResource;
    }

    Texture Context::uploadTexture(const loader::Texture& tex, const D3D12_CPU_DESCRIPTOR_HANDLE& handle, std::wstring_view name) {
        log::render->info(strings::encode(std::format(L"uploading texture {}", name)));
        
        const auto texWidth = UINT(tex.width);
        const auto texHeight = UINT(tex.height);
        const auto bpp = UINT(tex.bpp);
        const auto& data = tex.pixels;
        const auto format = DXGI_FORMAT_R8G8B8A8_UNORM; // TODO: this is configurable
        const auto desc = createResourceDesc(texWidth, texHeight, format);
        const auto textureDesc = createTextureUploadDesc(data.data(), texWidth, texHeight, bpp);
        const auto srvDesc = createSRVDesc(format);

        Com<ID3D12Resource> defaultResource;
        Com<ID3D12Resource> uploadResource;

        HRESULT hrDefault = device->CreateCommittedResource(
            &defaultProps, D3D12_HEAP_FLAG_NONE,
            &desc, D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr, IID_PPV_ARGS(&defaultResource)
        );
        check(hrDefault, "failed to create default resource");

        const auto textureSize = GetRequiredIntermediateSize(defaultResource.get(), 0, 1);
        const auto bufferSize = CD3DX12_RESOURCE_DESC::Buffer(textureSize);

        HRESULT hrUpload = device->CreateCommittedResource(
            &uploadProps, D3D12_HEAP_FLAG_NONE,
            &bufferSize, D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr, IID_PPV_ARGS(&uploadResource)
        );
        check(hrUpload, "failed to create upload resource");

        defaultResource.rename(name.data());
        uploadResource.rename(std::format(L"upload-texture-{}", name).data());

        UpdateSubresources(copyCommandList.get(), defaultResource.get(), uploadResource.get(), 0, 0, 1, &textureDesc);
    
        device->CreateShaderResourceView(defaultResource.get(), &srvDesc, handle);
    
        copyResources.push_back(uploadResource);

        return Texture({
            .texture = defaultResource,
            .mipLevels = 1,
            .format = format,
            .resolution = { LONG(texWidth), LONG(texHeight) }
        });
    }
}
