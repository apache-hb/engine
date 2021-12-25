#pragma once

#include "render/util.h"
#include "ndk/nvsdk_ngx.h"

#if ENABLE_DLSS
#   define DLSS_STUB ;
#else
#   define DLSS_STUB { }
#endif

namespace engine::render::ndk {
    struct Error : engine::Error {
        using Super = engine::Error;

        Error(NVSDK_NGX_Result result, std::string message, const std::source_location& location = std::source_location::current()) 
            : Super(message, location)
            , result(result) 
        { }

        virtual std::string query() const override;

        NVSDK_NGX_Result code() const { return result; }
    private:
        NVSDK_NGX_Result result;
    };

    struct DlssState {
        void create(ID3D12Device* device, std::wstring_view logs = L"dlss.log") DLSS_STUB
        void destroy() DLSS_STUB
    };
}
