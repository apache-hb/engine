#include "dlss.h"

#include "ndk/nvsdk_ngx.h"

/// TODO: what does this actually do?
#define APP_ID 231313132

namespace engine::render::ndk {
    void DlssState::create(ID3D12Device* device, std::wstring_view logs) {
        auto result = NVSDK_NGX_D3D12_Init(APP_ID, logs.data(), device);
    
        if (NVSDK_NGX_FAILED(result)) {
            if (result == NVSDK_NGX_Result_FAIL_FeatureNotSupported || result == NVSDK_NGX_Result_FAIL_PlatformError) {
                log::render->info("dlss not supported");
            } else {
                log::render->info("dlss failed to initialize");
            }
        }
    }

    void DlssState::destroy() {

    }
}
