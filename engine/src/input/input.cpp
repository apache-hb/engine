#include "engine/input/input.h"

#include "engine/base/win32.h"
#include "xinput.h"

using namespace engine;
using namespace engine::input;

using namespace math;

namespace {
    constexpr UINT kLeftDeadzone = XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE;
    constexpr UINT kRightDeadzone = XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE;
    constexpr float kTriggerDeadzone = XINPUT_GAMEPAD_TRIGGER_THRESHOLD;

    float2 stickRatio(float x, float y, float deadzone) {
        float magnitude = sqrt((x * x) + (y * y));

        if (magnitude > deadzone) {
            return float2 { x / SHRT_MAX, y / SHRT_MAX };
        } else {
            return float2::of(0.f);
        }
    }

    constexpr float triggerRatio(float it, float deadzone) {
        if (it > deadzone) {
            return it / 255.f;
        }

        return 0.f;
    }
}

Gamepad::Gamepad(size_t index) 
    : index(index) 
{ }

bool Gamepad::poll(Input *input) {
    XINPUT_STATE state;
    // controller not connected, nothing to update
    if (DWORD result = XInputGetState(DWORD(index), &state); result != ERROR_SUCCESS) {
        return false;
    }

    // no new packet, nothing to update
    if (!shouldSendPacket(state.dwPacketNumber)) {
        return false;
    }

    updatePacket(state.dwPacketNumber);

    float vertical = triggerRatio(state.Gamepad.bLeftTrigger, kTriggerDeadzone) - triggerRatio(state.Gamepad.bRightTrigger, kTriggerDeadzone);

    float2 movement = stickRatio(state.Gamepad.sThumbLX, state.Gamepad.sThumbLY, kLeftDeadzone);
    float2 rotation = stickRatio(state.Gamepad.sThumbRX, state.Gamepad.sThumbRY, kRightDeadzone);

    // no actual data to send
    if (vertical == 0.f && movement == 0.f && rotation == 0.f) {
        return false;
    }

    input->device = eGamepad;
    input->movement = movement.vec3(vertical);
    input->rotation = rotation;

    return true;
}

bool Gamepad::shouldSendPacket(size_t packet) const {
    if (packet >= lastPacket) {
        return true;
    }

    return false;
}

void Gamepad::updatePacket(size_t newPacket) {
    lastPacket = newPacket;
}
