#include "objects/common.h"

#include <comdef.h>

IDXGIFactory5 *simcoe::gFactory = nullptr;
ID3D12Debug *simcoe::gDxDebug = nullptr;
IDXGIDebug *simcoe::gDebug = nullptr;

std::string simcoe::hrErrorString(HRESULT hr) {
    _com_error err(hr);
    return err.ErrorMessage();
}
