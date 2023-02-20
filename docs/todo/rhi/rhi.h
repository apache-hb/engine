#pragma once

namespace simcoe::rhi {
    struct IFence {
        virtual ~IFence() = default;
    };

    struct IDescriptorSet {
        virtual ~IDescriptorSet() = default;
    };

    struct ICommandList {
        virtual ~ICommandList() = default;

        virtual void bind(IDescriptorSet *pDescriptorSet) = 0;
    };
    
    struct IQueue {
        virtual ~IQueue() = default;
    };

    struct IDevice {
        virtual ~IDevice() = default;

        virtual IQueue *newQueue() = 0;
        virtual ICommandList *newCommandList() = 0;
        virtual IDescriptorSet *newDescriptorSet() = 0;
    };
}
