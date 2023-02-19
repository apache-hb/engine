#pragma once

#include "simcoe/render/render.h"
#include "simcoe/core/win32.h"

#include "concurrentqueue.h"

namespace simcoe::render {
    struct Action {
        virtual ~Action() = default;

        virtual void execute(ID3D12GraphicsCommandList *pCommandList) = 0;
    };

    template<typename F>
    Action *newAction(F&& func) {
        struct Forward final : Action {
            Forward(F&& func)
                : func(std::forward<F>(func))
            { }

            void execute(ID3D12GraphicsCommandList *pCommandList) override {
                func(pCommandList);
            }

        private:
            F func;
        };

        return new Forward{ std::forward<F>(func) };
    }

    struct Queue {
        Queue(size_t size);

        void newQueue(ID3D12Device *pDevice);
        void deleteQueue();

        void wait();

        void push(Action *pCommand);

    private:
        moodycamel::ConcurrentQueue<Action*> queue;

        ID3D12CommandQueue *pQueue = nullptr;
        ID3D12CommandAllocator *pAllocator = nullptr;
        ID3D12GraphicsCommandList *pCommandList = nullptr;

        UINT64 fenceValue = 1;
        HANDLE fenceEvent = nullptr;
        ID3D12Fence *pFence = nullptr;
    };
}
