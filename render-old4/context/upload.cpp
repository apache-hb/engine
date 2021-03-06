#include "render/render.h"

using namespace engine::render;
using namespace engine::assets;

constexpr auto kDefaultProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
constexpr auto kUploadProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

constexpr D3D12_SUBRESOURCE_DATA createUploadDesc(const void *data, LONG_PTR size) {
    return { 
        .pData = data,
        .RowPitch = size,
        .SlicePitch = size
    };
}

constexpr D3D12_RESOURCE_DESC createResourceDesc(size_t width, size_t height, DXGI_FORMAT format) {
    return {
        .Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
        .Width = UINT(width),
        .Height = UINT(height),
        .DepthOrArraySize = 1,
        .MipLevels = 1,
        .Format = format,
        .SampleDesc = { 1, 0 }
    };
}

constexpr DXGI_FORMAT cvtFormat(Texture::Format format) {
    switch (format) {
        case Texture::Format::RGB: return DXGI_FORMAT_R8G8B8A8_UNORM;
        case Texture::Format::RGBA: return DXGI_FORMAT_R8G8B8A8_UNORM;
        default: return DXGI_FORMAT_UNKNOWN;
    }
}

constexpr size_t numChannels(Texture::Format format) {
    switch (format) {
        case Texture::Format::RGB: return 3;
        case Texture::Format::RGBA: return 4;
        default: return 0;
    }
}

Resource Context::uploadData(std::wstring_view name, const void* data, size_t size) {
    const auto bufferSize = CD3DX12_RESOURCE_DESC::Buffer(size);
    const auto uploadDesc = createUploadDesc(data, LONG_PTR(size));

    auto defaultResource = device.newCommittedResource(
        name, kDefaultProps, D3D12_HEAP_FLAG_NONE,
        bufferSize, D3D12_RESOURCE_STATE_COMMON
    );

    auto uploadResource = device.newCommittedResource(
        std::format(L"upload-{}", name), 
        kUploadProps, D3D12_HEAP_FLAG_NONE,
        bufferSize, D3D12_RESOURCE_STATE_GENERIC_READ
    );

    uploadResource.writeBytes(0, data, size);

    copyCommandList->CopyBufferRegion(defaultResource.get(), 0, uploadResource.get(), 0, size);

    copyResources.push_back(uploadResource);

    return defaultResource;
}

Resource Context::uploadTexture(const assets::Texture& texture) {
    const auto& [name, resolution, depth, data] = texture;
    const auto& [width, height] = resolution;
    const auto wname = strings::widen(name);
    const auto format = cvtFormat(depth);
    const auto channels = numChannels(depth);
    const auto desc = createResourceDesc(width, height, format);

    auto defaultResource = device.newCommittedResource(
        wname, kDefaultProps, D3D12_HEAP_FLAG_NONE,
        desc, D3D12_RESOURCE_STATE_COMMON
    );

    const auto [footprint, rowCount, rowSize, size] = device.getFootprint(defaultResource);
    const auto bufferSize = CD3DX12_RESOURCE_DESC::Buffer(size);

    auto uploadResource = device.newCommittedResource(
        std::format(L"upload-{}", wname), 
        kUploadProps, D3D12_HEAP_FLAG_NONE,
        bufferSize, D3D12_RESOURCE_STATE_GENERIC_READ
    );

    auto ptr = uploadResource.map<uint8_t>(0);

    for (UINT i = 0; i < rowCount; i++) {
        auto dst = ptr + rowSize * i;
        auto src = data.data() + width * channels * i;
        auto range = width * channels;

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

    copyCommandList->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);

    copyResources.push_back(uploadResource);

    return defaultResource;
}

void Context::finishCopy() {
    if (copyResources.empty()) {
        return;
    }

    check(copyCommandList->Close(), "failed to close copy command list");

    ID3D12CommandList* commandLists[] = { copyCommandList.get() };
    copyCommandQueue->ExecuteCommandLists(1, commandLists);

    check(copyCommandQueue->Signal(copyFence.get(), ++copyFenceValue), "failed to signal copy fence");
    check(copyFence->SetEventOnCompletion(copyFenceValue, copyFenceEvent), "failed to set fence event");
    WaitForSingleObject(copyFenceEvent, INFINITE);

    for (auto& resource : copyResources) {
        resource.tryDrop("copy-resource");
    }
    copyResources.clear();
}
