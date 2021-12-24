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

    constexpr D3D12_RESOURCE_DESC createResourceDesc(UINT width, UINT height, DXGI_FORMAT format) {
        return {
            .Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
            .Width = width,
            .Height = height,
            .DepthOrArraySize = 1,
            .MipLevels = 1,
            .Format = format,
            .SampleDesc = { 1, 0 },
            .Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR
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
    
    Resource Context::uploadBuffer(const void *data, size_t size, std::wstring_view name) {
        const auto bufferSize = CD3DX12_RESOURCE_DESC::Buffer(size);
        const auto uploadDesc = createUploadDesc(data, size);

        Resource defaultResource;
        Resource uploadResource;

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

        void *ptr = uploadResource.map(0, nullptr);
        memcpy(ptr, data, size);
        uploadResource.unmap(0);

        copyCommandList->CopyBufferRegion(defaultResource.get(), 0, uploadResource.get(), 0, size);

        copyResources.push_back(uploadResource);

        return defaultResource;
    }

    Resource Context::uploadTexture(const Image& tex, std::wstring_view name) {
        log::render->info(strings::encode(std::format(L"uploading texture {}", name)));
        
        const auto& [width, height, component, data] = tex;
        const auto desc = createResourceDesc(UINT(width), UINT(height), DXGI_FORMAT_R8G8B8A8_UNORM);

        Resource defaultResource;
        Resource uploadResource;

        /// create our staging and destination resource

        HRESULT hrDefault = device->CreateCommittedResource(
            &defaultProps, D3D12_HEAP_FLAG_NONE,
            &desc, D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr, IID_PPV_ARGS(&defaultResource)
        );
        check(hrDefault, "failed to create default resource");

        const auto [footprint, rowCount, rowSize, size] = device.getFootprint(defaultResource);
        const auto bufferSize = CD3DX12_RESOURCE_DESC::Buffer(size);

        HRESULT hrUpload = device->CreateCommittedResource(
            &uploadProps, D3D12_HEAP_FLAG_NONE,
            &bufferSize, D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr, IID_PPV_ARGS(&uploadResource)
        );
        check(hrUpload, "failed to create upload resource");

        /// rename our resources to be kind to the debugger

#if D3D12_DEBUG
        defaultResource.rename(name.data());
        uploadResource.rename(std::format(L"upload-texture-{}", name).data());
#endif

        /// copy over our data into the staging buffer
        auto ptr = reinterpret_cast<uint8_t*>(uploadResource.map(0));

        for (auto i = 0u; i < rowCount; i++) {
            auto dst = ptr + rowSize * i;
            auto src = data.data() + width * component * i;
            auto range = width * component;

            memcpy(dst, src, range);
        }

        uploadResource.unmap(0);

        D3D12_TEXTURE_COPY_LOCATION dst = {
            .pResource = defaultResource.get(),
            .Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
            .SubresourceIndex = 0
        };

        D3D12_TEXTURE_COPY_LOCATION src = {
            .pResource = uploadResource.get(),
            .Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT,
            .PlacedFootprint = footprint
        };

        // record our copy for this texture
        copyCommandList->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);

        // mark the upload buffer for discarding
        // TODO: eventually this might be cached instead
        copyResources.push_back(uploadResource);

        return defaultResource;
    }
}
