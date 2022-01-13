#pragma once

#include <functional>

#include "system/system.h"
#include "objects/device.h"
#include "atomic-queue/queue.h"

namespace engine::render {
    struct Factory;
    struct Adapter;
    struct Context;

    namespace Allocators {
        enum Index : size_t {
            Copy, /// allocator used for copy commands
            ImGui, /// allocator used for imgui commands
            Total
        };
    }

    using Event = std::function<void(Context*)>;

    struct Context {
        struct Budget {
            size_t queueSize;
        };

        struct Create {
            // render device settings
            Factory* factory;
            size_t adapter;
            
            // render output settings
            system::Window* window;
            Resolution resolution;

            // render config settings
            UINT buffers;
            bool vsync;

            Budget budget;
        };

        Context(Create&& create);

        void create();
        void destroy();

        void present();

        void addEvent(Event&& event);

    private:
        // api agnostic data and creation data
        Create info;
        ubn::queue<Event> events;

        Factory& getFactory();
        Adapter& getAdapter();

        Resolution getSceneResolution();
        Resolution getPostResolution();

        // d3d12 api objects
        void createDevice();
        void destroyDevice();

        void createViews();

        Device device;


        // our internal resolution
        // by extension this is the resolution of our intermediate target
        View sceneView;
        Resolution sceneResolution;

        // our extern resolution
        // by extension this is the resolution of our swapchain
        // this should also be the resolution of the actual window
        View postView;
        Resolution postResolution;
    };
}
