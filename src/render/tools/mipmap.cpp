#define NOMINMAX

#include "mipmap.h"

#include <array>

namespace engine::render {
    constexpr ComputeLibrary::Create toolCreate = {
        .path = L"resources\\shaders\\mipmap-tool.hlsl",
        .csMain = "genMipMaps"
    };

    constexpr D3D12_ROOT_SIGNATURE_FLAGS rootFlags =
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS       |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS     |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS   |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS      ;

    MipMapTool::MipMapTool(Device<ID3D12Device4> device): device(device), library(toolCreate) { 
        std::array<CD3DX12_DESCRIPTOR_RANGE1, 2> ranges;
        std::array<CD3DX12_ROOT_PARAMETER1, 3> params;

        ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);
        ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0);

        params[0].InitAsConstants(2, 0);
        params[1].InitAsDescriptorTable(1, &ranges[0]);
        params[2].InitAsDescriptorTable(1, &ranges[1]);
        
        D3D12_STATIC_SAMPLER_DESC samplers[] = {{
            .Filter = D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT,
            .AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
            .AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
            .AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
            .MaxAnisotropy = 0,
            .ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER,
            .BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK,
            .MinLOD = 0.0f,
            .MaxLOD = D3D12_FLOAT32_MAX,
            .ShaderRegister = 0,
            .RegisterSpace = 0,
            .ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL
        }};

        auto signature = compileRootSignature({
            .version = device.getHighestRootVersion(),
            .params = { params },
            .samplers = { samplers },
            .flags = rootFlags
        });

        rootSignature = device.newRootSignature(L"mipmap-root-signature", signature);
        pipeline = device.newComputePSO(L"mipmap-pso", library, rootSignature.get());

        check(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COMPUTE, IID_PPV_ARGS(&allocator)), "failed to create command allocator");
        computeList = device.newCommandList(L"mipmap-command-list", D3D12_COMMAND_LIST_TYPE_COMPUTE, allocator.get(), pipeline.get());
        computeQueue = device.newCommandQueue(L"mipmap-command-queue", { D3D12_COMMAND_LIST_TYPE_COMPUTE });
    }

    void MipMapTool::createMipMaps(std::vector<Texture> textures) {
        auto heap = device.newHeap(L"mipmap-heap", {
            .Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
            .NumDescriptors = UINT(textures.size() * 2), /// 1 for input, 1 for output
            .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
        });

        ID3D12DescriptorHeap* heaps[] = { heap.get() };

        computeList->SetComputeRootSignature(rootSignature.get());
        computeList->SetPipelineState(pipeline.get());
        computeList->SetDescriptorHeaps(UINT(std::size(heaps)), heaps);

        size_t size = textures.size();
        submit.resize(size);

        for (size_t i = 0; i < size; i++) {
            auto& texture = textures[i];
            if (texture.mipLevels == 1) { continue; }

            auto srvCpuHandle = heap.cpuHandle(i);
            auto uavCpuHandle = heap.cpuHandle(i + 1);

            auto srvGpuHandle = heap.gpuHandle(i);
            auto uavGpuHandle = heap.gpuHandle(i + 1);

            for (size_t topMip = 0; topMip < texture.mipLevels - 1; topMip++) {
                auto mip = topMip + 1;
                auto width = std::max<LONG>(1, texture.resolution.width >> mip);
                auto height = std::max<LONG>(1, texture.resolution.height >> mip);
            
                D3D12_SHADER_RESOURCE_VIEW_DESC srcDesc = {
                    .Format = texture.format,
                    .ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D,
                    .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
                    .Texture2D = {
                        .MostDetailedMip = UINT(mip),
                        .MipLevels = UINT(topMip),
                    }
                };

                D3D12_UNORDERED_ACCESS_VIEW_DESC dstDesc = {
                    .Format = texture.format,
                    .ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D,
                    .Texture2D = { .MipSlice = UINT(mip) }
                };

                device->CreateShaderResourceView(texture.get(), &srcDesc, srvCpuHandle);
                device->CreateUnorderedAccessView(texture.get(), nullptr, &dstDesc, uavCpuHandle);

                computeList->SetComputeRoot32BitConstant(0, UINT(1.f / width), 0);
                computeList->SetComputeRoot32BitConstant(0, UINT(1.f / height), 1);
            
                computeList->SetComputeRootDescriptorTable(1, srvGpuHandle);
                computeList->SetComputeRootDescriptorTable(2, uavGpuHandle);

                computeList->Dispatch(std::max<UINT>(width / 8, 1), std::max<UINT>(height / 8, 1), 1);
                
                auto transition = CD3DX12_RESOURCE_BARRIER::UAV(texture.get());
                computeList->ResourceBarrier(1, &transition);
            }

            auto transition = CD3DX12_RESOURCE_BARRIER::Transition(texture.get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
            computeList->ResourceBarrier(1, &transition);

            submit[i] = texture;
        }

        check(computeList->Close(), "failed to close compute list");
    }
}
