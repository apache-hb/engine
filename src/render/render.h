#pragma once

#include "factory.h"

#include "system/system.h"

namespace engine::render {
    namespace CbvResources {
        enum Index : int {
            Intermediate,
            ImGui,
            Total
        };
    }

    struct Context {
        /// information needed to create a render context
        struct Create {
            /// all our physical devices and which one we want to use
            Factory factory;

            size_t currentAdapter;

            /// the window we want to display to
            system::Window* window;

            /// the number of back buffers we want to use by default
            UINT backBuffers;

            /// the resolution we want to use
            Resolution resolution;
        };

        Context(Create&& create);
        ~Context();

        ///
        /// settings managment
        ///

        /**
         * @brief change our currently used device
         * 
         * @param index the index of the device we want to use
         */
        void setDevice(size_t index);

        /**
         * @brief change the number of back buffers used
         * 
         * @param buffers the number of back buffers we want to use
         */
        void setBackBuffers(UINT buffers);

        /**
         * @brief change the resolution of the window
         * 
         * @param newResolution the new resolution
         */
        void setResolution(Resolution newResolution);


        /// 
        /// rendering
        ///

        /**
         * @brief redraw and present to screen
         * 
         * @return false if the context was lost and failed to be reacquired.
         *         if this happens exit the render loop and quit, this is fatal.
         */
        bool present() noexcept;

    private:
        ///
        /// create and destroy methods. context/lifetime.cpp
        ///

        /// create or destroy resources that depend on the adapter
        /// this is basically everything as the device depends on the adapter
        void createDevice();
        void destroyDevice();

        /// resize viewport and scissor rects
        void buildViews();

        /// create or destroy resources that depend on the number of back buffers
        /// mostly just the backbuffers
        void createBuffers();
        void destroyBuffers();

        /// create or destroy resources that depend on the window resolution
        /// this is things such as the intermediate render target
        void createResolutionResources();
        void destroyResolutionResources();



        ///
        /// resource access methods. context/access.cpp
        ///

        Factory& getFactory();
        Adapter& getAdapter();
        system::Window& getWindow();
        Resolution getCurrentResolution() const;
        DXGI_FORMAT getFormat() const;
        UINT getBackBufferCount() const;



        ///
        /// calucated resource access methods. context/access.cpp
        ///

        UINT requiredRtvHeapSize() const;
        UINT requiredCbvHeapSize() const;

        Object<ID3D12Resource>& getIntermediateTarget();
        CD3DX12_CPU_DESCRIPTOR_HANDLE getIntermediateRtvHandle();
        CD3DX12_CPU_DESCRIPTOR_HANDLE getIntermediateCbvHandle();



        ///
        /// all context data
        ///

        /// per-frame information
        struct FrameData {
            /// the back buffer resource we are currently rendering to
            Object<ID3D12Resource> finalTarget;

            /// our fence value for the current backbuffer
            UINT64 fenceValue;
        };

        /// our creation info
        Create info;

        /// current internal and display resolution
        Resolution sceneResolution;
        Resolution displayResolution;

        /// current internal and display viewport and scissor
        View sceneView;
        View postView;

        /// our current device
        Device<ID3D12Device4> device;

        /// all of the command queues we need
        Object<ID3D12CommandQueue> directCommandQueue;
        Object<ID3D12CommandQueue> copyCommandQueue;

        /// our swapchain
        Com<IDXGISwapChain3> swapchain;
        UINT frameIndex;
        
        /// all our render targets and the intermediate target
        DescriptorHeap rtvHeap;
        DescriptorHeap cbvHeap;
        Object<ID3D12Resource> intermediateRenderTarget;

        /// our frame data
        FrameData *frameData;

        /// sync objects
        Object<ID3D12Fence> fence;
        HANDLE fenceEvent;
    };
}
