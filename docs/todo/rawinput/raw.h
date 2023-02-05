#pragma once

#include "simcoe/core/util.h"
#include "simcoe/core/win32.h"
#include "simcoe/input/input.h"

#include <memory>
#include <unordered_set>

namespace simcoe::input {
    struct IRawDevice : ISource {
        friend struct RawDevicePool;

        HANDLE getHandle() const { return hDevice; }

    protected:
        virtual void update(PRAWINPUT pInput) = 0;

        IRawDevice(Device kind, HANDLE hDevice) 
            : ISource(kind)
            , hDevice(hDevice) 
        { }

    private:
        // device constants
        HANDLE hDevice;
    };

    struct RawMouse final : IRawDevice {
        friend struct RawDevicePool;

        bool poll(State& state) override;

    private:
        RawMouse(HANDLE hDevice);

        void update(PRAWINPUT pInput) override;

        KeyState keys = {};
        
        math::float2 base = {};
        math::float2 absolute = {};

        size_t index = 0;
    };

    struct RawKeyboard final : IRawDevice {
        friend struct RawDevicePool;

        bool poll(State& state) override;

    private:
        RawKeyboard(HANDLE hDevice);
        void update(PRAWINPUT pInput) override;
    };

    struct RawDevicePool final : ISource {
        using DeviceSet = std::unordered_set<std::unique_ptr<IRawDevice>>;

        RawDevicePool();

        void update(UINT msg, WPARAM wparam, LPARAM lparam);

        virtual bool poll(State& state) override;

    private:
        void onEvent(DWORD type, PRAWINPUT pInput);
        bool sendEvent(DWORD type, HANDLE hDevice, PRAWINPUT pInput);
        void addDevice(DWORD type, HANDLE hDevice);
        
        void addMouse(HANDLE hDevice);
        void addKeyboard(HANDLE hDevice);

        // raw input data buffer
        std::vector<std::byte> buffer;

        // output devices
        std::array<DeviceSet, RIM_TYPEMAX> devices;
        
        // error reporting
        util::DoOnce noInput;
        util::DoOnce dataSize;
        util::DoOnce invalidType;
    };
}
