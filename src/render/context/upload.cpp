#include "render/render.h"

using namespace engine::render;

constexpr auto kDefaultProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
constexpr auto kUploadProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

constexpr D3D12_SUBRESOURCE_DATA createUploadDesc(const void *data, LONG_PTR size) {
    return { 
        .pData = data,
        .RowPitch = size,
        .SlicePitch = size
    };
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
