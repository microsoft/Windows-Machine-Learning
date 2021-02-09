//
// pch.h
// Precompiled header for commonly included header files
//

#pragma once

#define NOMINMAX
#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING 1 // The C++ Standard doesn't provide equivalent non-deprecated functionality yet.

#include <vcruntime.h>
#include <windows.h>

#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.AI.MachineLearning.h>
#include <winrt/Windows.Media.h>
#include <winrt/Windows.Storage.h>
#include <winrt/Windows.Graphics.h>
#include <winrt/Windows.Graphics.Imaging.h>

#ifdef USE_WINML_NUGET
#include "winrt/Microsoft.AI.MachineLearning.h"
#endif

#include <string>
#include <codecvt>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <numeric>

using convert_type = std::codecvt_utf8<wchar_t>;
using wstring_to_utf8 = std::wstring_convert<convert_type, wchar_t>;
