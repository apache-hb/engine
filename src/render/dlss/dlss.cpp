#include "dlss.h"

#include "util/strings.h"

/// TODO: what does this actually do?
#define APP_ID 231313132

namespace engine::render::ndk {
    std::string toString(NVSDK_NGX_Result result) {
        return strings::encode(GetNGXResultAsString(result));
    }

    void nvCheck(NVSDK_NGX_Result result, std::string_view message, std::source_location location = std::source_location::current()) {
        if (NVSDK_NGX_FAILED(result)) { throw Error(result, std::string(message), location); }
    }

    std::string Error::query() const {
        return std::format("error: {}\n{}", toString(result), Super::query());
    }

    void DlssState::create(ID3D12Device* device, std::wstring_view logs) {
        auto result = NVSDK_NGX_D3D12_Init(APP_ID, logs.data(), device);
    
        if (NVSDK_NGX_FAILED(result)) {
            if (result == NVSDK_NGX_Result_FAIL_FeatureNotSupported || result == NVSDK_NGX_Result_FAIL_PlatformError) {
                log::render->info("dlss not supported: {}\n{}", (int)result, toString(result));
            } else {
                log::render->info("dlss failed to initialize: {}\n{}", (int)result, toString(result));
            }
        }
    }

    void DlssState::destroy() {

    }
}
