#pragma once

#include "common.h"

namespace engine {
    struct DxFence final : rhi::Fence {
        DxFence(ID3D12Fence *fence, HANDLE event);
        ~DxFence() override;

        void waitUntil(size_t signal) override;

        ID3D12Fence *get();

    private:
        UniqueComPtr<ID3D12Fence> fence;
        HANDLE event;
    };
}
