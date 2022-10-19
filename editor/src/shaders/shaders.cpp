#include "editor/shaders/shaders.h"

#include "engine/base/logging.h"
#include "engine/base/util.h"
#include "engine/base/panic.h"

#include "engine/container/com/com.h"

#include <algorithm>
#include <d3dcompiler.h>
#include <dxc/d3d12shader.h>
#include <dxc/dxcapi.h>

using namespace simcoe;
using namespace editor;

namespace {
    com::UniqueComPtr<IDxcUtils> gUtils;
    com::UniqueComPtr<IDxcCompiler> gCompiler;

    LPCWSTR getTargetEntry(shaders::Target target) {
        switch (target) {
        case editor::shaders::eVertexShader: return L"-EvsMain";
        case editor::shaders::ePixelShader: return L"-EpsMain";
        default: ASSERTM(false, "Invalid target");
        }
    } 

    LPCWSTR getTargetProfile(shaders::Target target) {
        switch (target) {
        case editor::shaders::eVertexShader: return L"vs_6_0";
        case editor::shaders::ePixelShader: return L"ps_6_0";
        default: ASSERTM(false, "Invalid target");
        }
    }
}

void shaders::init() {
    DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&gUtils));
    DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&gCompiler));

}

std::vector<std::byte> shaders::compile(const std::string& text, shaders::Target target, bool debug) {
    auto& channel = logging::get(logging::eRender);

    com::UniqueComPtr<IDxcBlobEncoding> source;
    com::UniqueComPtr<IDxcResult> result;
    com::UniqueComPtr<IDxcBlobUtf8> errors;

    gUtils->CreateBlob(text.c_str(), UINT32(text.length()), CP_UTF8, &source);

    std::vector<LPCWSTR> args = {
        getTargetEntry(target),
        getTargetProfile(target),
        DXC_ARG_WARNINGS_ARE_ERRORS
    };

    if (debug) {
        args.push_back(DXC_ARG_DEBUG);
        args.push_back(L"-Qembed_debug");
        args.push_back(L"-DDEBUG=1");
    } else {
        args.push_back(DXC_ARG_OPTIMIZATION_LEVEL3);
    }

    DxcBuffer buffer {
        .Ptr = source->GetBufferPointer(),
        .Size = source->GetBufferSize(),
        .Encoding = CP_UTF8
    };

    gCompiler->Compile(
        /* pSource = */ &buffer, 
        /* pArguments = */ args.data(), 
        /* argCount = */ uint32_t(args.size()), 
        /* pIncludeHandle = */ nullptr,
        /* riid = */ IID_PPV_ARGS(&result)
    );

    result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr);
    if (errors && errors->GetStringLength() > 0) {
        channel.warn("Shader compilation failed: {}", errors->GetStringPointer());
        return {};
    }

    com::UniqueComPtr<IDxcBlob> blob;
    result->GetResult(&blob);

    std::vector<std::byte> out;

    std::copy(
        reinterpret_cast<std::byte*>(blob->GetBufferPointer()), 
        reinterpret_cast<std::byte*>(blob->GetBufferPointer()) + blob->GetBufferSize(), 
        std::back_inserter(out)
    );

    return out;
}
