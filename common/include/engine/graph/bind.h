#pragma once

#include "engine/graph/resource.h"

namespace simcoe::graph {
    struct Output {
        Output() = default;
        Output(auto **data, rhi::Buffer::State state = rhi::Buffer::State::eInvalid) 
            : data((RenderResource**)data)
            , state(state)
        { }

        virtual ~Output() = default;

        virtual RenderResource *get() { return *data; }
        rhi::Buffer::State getState() { return state; }

    protected:
        RenderResource **data = nullptr;
        rhi::Buffer::State state;
    };

    struct Input {
        Input() = default;
        Input(auto **data, rhi::Buffer::State state = rhi::Buffer::State::eInvalid) 
            : data((RenderResource**)data)
            , state(state) 
        { }

        virtual ~Input() = default;

        virtual RenderResource *get() { return *data; }
        rhi::Buffer::State getState() { return state; }

    private:
        RenderResource **data = nullptr;
        rhi::Buffer::State state;
    };

    struct VectorOutput : Output {
        VectorOutput() = default;
        VectorOutput(auto **data, size_t *index, rhi::Buffer::State state = rhi::Buffer::State::eInvalid) 
            : Output(data, state)
            , index(index)
        { }

        RenderResource *get() override { return data[*index]; }

    private:
        size_t *index;
    };
}
