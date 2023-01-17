#pragma once

#include "binds.h"
#include "util/win32.h"

namespace engine::input {
    struct MnK {
        MnK(float sensitivity, float shiftAccel): sensitivity(sensitivity), shiftAccel(shiftAccel) { 
            memset(keys, 0, sizeof(keys));
        }

        void pushPress(WPARAM wparam);
        void pushRelease(WPARAM wparam);
    
        Input poll();
    private:
        int mouseX = 0;
        int mouseY = 0;

        int newX = 0;
        int newY = 0;

        float sensitivity;
        float shiftAccel;

        bool keys[0xFF] = {};
    };
}
