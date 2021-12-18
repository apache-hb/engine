#pragma once

#include "binds.h"
#include "util/win32.h"

namespace engine::input {
    struct MnK {
        MnK(float sensitivity)
            : sensitivity(sensitivity)
        { }

        void pushPress(WPARAM wparam);
        void pushRelease(WPARAM wparam);
    
        Input poll();
    private:
        int mouseX = 0;
        int mouseY = 0;

        int newX = 0;
        int newY = 0;

        float sensitivity;

        bool keys[0xFF] = {};
    };
}
