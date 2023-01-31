#include "simcoe/core/win32.h"

extern "C" { 
    // load up the agility sdk
    DLLEXPORT extern const UINT D3D12SDKVersion = 600; 
    DLLEXPORT extern const auto* D3D12SDKPath = u8".\\resources\\agility"; 
}
