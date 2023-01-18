#include "simcoe/core/macros.h"

#include <windows.h>

extern "C" { 
    // load up the agility sdk
    DLLEXPORT extern const UINT D3D12SDKVersion = 600; 
    DLLEXPORT extern const auto* D3D12SDKPath = u8".\\resources\\agility"; 
}
