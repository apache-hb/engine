#pragma once

#include "factory.h"

#include "system/system.h"

namespace engine::render {
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

        Context(Create create);
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
        Create info;
    };
}
