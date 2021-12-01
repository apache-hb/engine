#include "util/win32.h"

extern "C" { 
    __declspec(dllexport) extern const UINT D3D12SDKVersion = 4;
    __declspec(dllexport) extern const auto* D3D12SDKPath = u8".\\D3D12\\"; 
}
