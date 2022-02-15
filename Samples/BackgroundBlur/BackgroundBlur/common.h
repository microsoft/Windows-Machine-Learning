#pragma once
#include <winrt/Windows.Foundation.h>
#include <pix3.h>

#define CHECK_HR(hr) if (FAILED(hr)) { goto done; }

