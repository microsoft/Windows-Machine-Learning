#pragma once
#include <winrt/Windows.Foundation.h>

#define CHECK_HR(hr) if (FAILED(hr)) { goto done; }
