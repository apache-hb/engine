#pragma once

#include "render/objects/compute.h"
#include "render/objects/device.h"
#include "render/objects/texture.h"

namespace engine::render {
    struct MipMapTool {
		MipMapTool() = default;
        MipMapTool(Device<ID3D12Device4> device);

		void createMipMaps(std::vector<Texture> textures);

		template<typename F>
		std::vector<Texture> flush(F&& func) {
			ID3D12CommandList* lists[] = { computeList.get() };
			computeQueue->ExecuteCommandLists(UINT(std::size(lists)), lists);
			func(computeQueue);

			return submit;
		}
    private:
		Device<ID3D12Device4> device;
        ComputeLibrary library;

        Com<ID3D12RootSignature> rootSignature;
        Com<ID3D12PipelineState> pipeline;

        Com<ID3D12CommandAllocator> allocator;
        Com<ID3D12GraphicsCommandList> computeList;
        Com<ID3D12CommandQueue> computeQueue;

		std::vector<Texture> submit;
    };
}
