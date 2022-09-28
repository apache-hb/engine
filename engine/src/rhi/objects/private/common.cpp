#include "objects/common.h"

#include <comdef.h>

IDXGIFactory5 *engine::gFactory = nullptr;
ID3D12Debug *engine::gDxDebug = nullptr;
IDXGIDebug *engine::gDebug = nullptr;

std::string engine::hrErrorString(HRESULT hr) {
    _com_error err(hr);
    return err.ErrorMessage();
}