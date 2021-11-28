#include <vector>

#include "util.h"

namespace render {
    namespace dxgi {
        using Factory4 = Ptr<IDXGIFactory4>;
        using Adapter1 = Ptr<IDXGIAdapter1>;
    }

    namespace d3d12 {
        using Device1 = Ptr<ID3D12Device1>;
        using CommandQueue = Ptr<ID3D12CommandQueue>;
        using InfoQueue1 = Ptr<ID3D12InfoQueue1>;
        using Debug = Ptr<ID3D12Debug>;
        using Fence = Ptr<ID3D12Fence>;
    }

    struct Instance {
        HRESULT create();
        void destroy();

    private:
        friend struct Context;

        d3d12::Debug debug;
        dxgi::Factory4 factory;
        std::vector<dxgi::Adapter1> adapters;
    };

    struct Context {
        HRESULT create(const Instance &instance, size_t index);
        void destroy();

    private:
        /// immutable instance data
        dxgi::Factory4 factory;
        dxgi::Adapter1 adapter;

        // core context data created from the instance
        d3d12::InfoQueue1 infoQueue;
        d3d12::Device1 device;
        d3d12::CommandQueue commandQueue;

        // synchronization objects
        UINT frameIndex;
        HANDLE fenceEvent;
        d3d12::Fence fence;
        UINT64 *fenceValues;
    };

}
