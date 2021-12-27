#pragma once

#include "factory.h"
#include "objects/library.h"
#include "objects/commands.h"
#include "objects/resources.h"
#include "system/system.h"

namespace engine::render {
    namespace CbvResources {
        enum Index : int {
            Intermediate,
            ImGui,
            Total
        };
    }

    namespace Allocator {
        enum Index : int {
            Direct,
            Copy,
            Total
        };

        constexpr D3D12_COMMAND_LIST_TYPE type(Index index) {
            switch (index) {
            case Direct: return commands::kDirect;
            case Copy: return commands::kCopy;
            default: throw engine::Error("invalid allocator index");
            }
        }

        constexpr std::wstring_view name(Index index) {
            switch (index) {
            case Direct: return L"direct";
            case Copy: return L"copy";
            default: throw engine::Error("invalid allocator index");
            }
        }
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
        /// resource uploading
        ///

        template<typename T>
        VertexBuffer uploadVertexBuffer(std::wstring_view name, std::span<const T> data) {
            return uploadVertexData(name, data.data(), UINT(data.size() * sizeof(T)), sizeof(T));
        }


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

        void populate();

#pragma region lifetime management (context/lifetime.cpp)

        /// create or destroy resources that depend on nothing
        void createContext();
        void destroyContext();

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

        /// create or destroy copy pipeline
        void createCopyPipeline();
        void destroyCopyPipeline();

        /// create or destroy pipeline objects
        void createPipeline();
        void destroyPipeline();



#pragma region sync methods (context/context.cpp)

        void waitForGpu(Queue queue);
        void nextFrame(Queue queue);



#pragma region resource uploading methods (context/upload.cpp)

        Resource uploadData(std::wstring_view name, const void* data, UINT size);
        VertexBuffer uploadVertexData(std::wstring_view name, const void* data, UINT size, UINT stride);


#pragma region resource access (context/access.cpp)

        Factory& getFactory();
        Adapter& getAdapter();
        system::Window& getWindow();
        Resolution getCurrentResolution() const;
        DXGI_FORMAT getFormat() const;
        UINT getBackBufferCount() const;



#pragma region calculated resource access (context/access.cpp)

        UINT requiredRtvHeapSize() const;
        UINT requiredCbvHeapSize() const;
        UINT getCurrentFrameIndex() const;

        Resource& getIntermediateTarget();
        CD3DX12_CPU_DESCRIPTOR_HANDLE getIntermediateRtvHandle();

        CD3DX12_CPU_DESCRIPTOR_HANDLE getIntermediateCbvCpuHandle();
        CD3DX12_GPU_DESCRIPTOR_HANDLE getIntermediateCbvGpuHandle();

        CD3DX12_CPU_DESCRIPTOR_HANDLE getRenderTargetCpuHandle(UINT index);
        CD3DX12_GPU_DESCRIPTOR_HANDLE getRenderTargetGpuHandle(UINT index);

        Object<ID3D12CommandAllocator> getAllocator(Allocator::Index type, size_t index = SIZE_MAX);


        ///
        /// all context data
        ///

        /// per-frame information
        struct FrameData {
            /// the back buffer resource we are currently rendering to
            Resource target;

            /// all of this frames allocators
            Object<ID3D12CommandAllocator> allocators[Allocator::Total];

            /// our fence value for the current backbuffer
            UINT64 fenceValue;
        };

        /// our creation info
        Create info;



#pragma region managed by buildViews()

        /// current internal and display resolution
        Resolution sceneResolution;
        Resolution displayResolution;

        /// current internal and display viewport and scissor
        View sceneView;
        View postView;



#pragma region managed by createDevice()

        /// our current device
        Device<ID3D12Device4> device;
        Queue directCommandQueue;

#pragma region managed by createBuffers()

        /// our swapchain
        Com<IDXGISwapChain3> swapchain;
        UINT frameIndex;
        
        /// all our render targets and the intermediate target
        DescriptorHeap rtvHeap;
        DescriptorHeap cbvHeap;
        Resource intermediateRenderTarget;

        /// our frame data
        FrameData *frameData;

        /// sync objects
        Object<ID3D12Fence> fence;
        HANDLE fenceEvent;



#pragma region managed by createPipeline()

        /// all resources needed for upscaling
        ShaderLibrary shaders;
        Commands directCommands;
        VertexBuffer screenBuffer;
        RootSignature rootSignature;
        PipelineState pipelineState;


#pragma region managed by createCopyPipeline()

        Queue copyCommandQueue;
        Commands copyCommands;
        std::vector<Resource> copyResources;
    };
}
