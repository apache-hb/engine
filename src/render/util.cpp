#include "util.h"

#include "util/strings.h"

#include <comdef.h>
#include <d3dcompiler.h>

extern "C" {
    __declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}

namespace engine::render {
    std::string toString(HRESULT hr) {
        return strings::encode(_com_error(hr).ErrorMessage());
    }

    void check(HRESULT hr, const std::string& message, std::source_location location) {
        if (FAILED(hr)) { throw render::Error(hr, message, location); }
    }

    Output::Output(IDXGIOutput *output): self(output) {
        check(self->GetDesc(&desc), "failed to get output description");
    }

    std::string Output::name() const {
        return strings::encode(desc.DeviceName);
    }

    RECT Output::coords() const {
        return desc.DesktopCoordinates;
    }

    Adapter::Adapter(IDXGIAdapter1 *adapter): self(adapter) { 
        if (!adapter) { return; }

        check(self->GetDesc1(&desc), "failed to get adapter description");
    
        IDXGIOutput *output;
        for (UINT i = 0; adapter->EnumOutputs(i, &output) != DXGI_ERROR_NOT_FOUND; ++i) {
            displays.push_back(Output(output));
        }
    }

    std::string Adapter::name() const {
        return strings::encode(desc.Description);
    }

    units::Memory Adapter::videoMemory() const {
        return units::Memory(desc.DedicatedVideoMemory);
    }

    units::Memory Adapter::systemMemory() const {
        return units::Memory(desc.DedicatedSystemMemory);
    }

    units::Memory Adapter::sharedMemory() const {
        return units::Memory(desc.SharedSystemMemory);
    }

    std::span<Output> Adapter::outputs() {
        return std::span{displays};
    }

    bool Adapter::software() const {
        return desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE;
    }

    Factory::Factory(bool debug) {
        UINT flags = debug ? DXGI_CREATE_FACTORY_DEBUG : 0;

        check(CreateDXGIFactory2(flags, IID_PPV_ARGS(&self)), "failed to create DXGI factory");

        IDXGIAdapter1 *adapter;
        for (UINT i = 0; self->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND; i++) {
            devices.push_back(Adapter(adapter));
        }

        if (auto factory5 = self.as<IDXGIFactory5>(); factory5.valid()) {
            check(factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &tearing, sizeof(tearing)), "failed to check for tearing support");
        }
    }

    Com<IDXGISwapChain1> Factory::createSwapChain(Com<ID3D12CommandQueue> queue, HWND window, const DXGI_SWAP_CHAIN_DESC1 &desc) {
        Com<IDXGISwapChain1> swapChain;
        check(self->CreateSwapChainForHwnd(queue.get(), window, &desc, nullptr, nullptr, &swapChain), "failed to create swap chain");
        check(self->MakeWindowAssociation(window, DXGI_MWA_NO_ALT_ENTER), "failed to make window association");
        return swapChain;
    }

    std::span<Adapter> Factory::adapters() {
        return std::span{devices};
    }

    Adapter Factory::adapter(size_t index) { 
        return devices[index];
    }

    UINT Factory::presentFlags() const {
        return tearing ? DXGI_PRESENT_ALLOW_TEARING : 0;
    }

#if D3D12_DEBUG
#   define COMPILE_FLAGS D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION
#else
#   define COMPILE_FLAGS 0
#endif

    Com<ID3DBlob> compileShader(std::wstring_view path, std::string_view entry, std::string_view target) {
        Com<ID3DBlob> shader;
        Com<ID3DBlob> error;

        HRESULT hr = D3DCompileFromFile(path.data(), nullptr, nullptr, entry.data(), target.data(), COMPILE_FLAGS, 0, &shader, &error);
        if (FAILED(hr)) {
            auto msg = error.valid() ? (const char*)error->GetBufferPointer() : "no error message";
            throw render::Error(hr, std::format("failed to compile shader {} {}\n{}", entry, target, msg));
        }

        return shader;
    }
}
