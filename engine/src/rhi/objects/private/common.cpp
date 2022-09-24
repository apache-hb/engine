#include "objects/common.h"

#include <comdef.h>

std::string engine::hrErrorString(HRESULT hr) {
    _com_error err(hr);
    return err.ErrorMessage();
}