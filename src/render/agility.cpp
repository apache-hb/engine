#include "util/win32.h"

#define DLLEXPORT __declspec(dllexport)

extern "C" { 
    DLLEXPORT extern const UINT D3D12SDKVersion = 600; 
    DLLEXPORT extern const auto* D3D12SDKPath = u8".\\D3D12\\"; 
}
