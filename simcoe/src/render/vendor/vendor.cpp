#include "simcoe/core/macros.h"

#include <windows.h>

extern "C" { 
    // ask vendors to use the high performance gpu if we have one
    DLLEXPORT extern const DWORD NvOptimusEnablement = 0x00000001;
    DLLEXPORT extern int AmdPowerXpressRequestHighPerformance = 1;
}
