//
// pch.h
// Precompiled header for commonly included header files
//

#pragma once

#define NOMINMAX
#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING 1 // The C++ Standard doesn't provide equivalent non-deprecated functionality yet.
#define _SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING

#include <vcruntime.h>
#include <windows.h>
#include <winstring.h>

#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.AI.MachineLearning.h>
#include <winrt/Windows.Media.h>
#include <winrt/Windows.Storage.h>
#include <winrt/Windows.Storage.Streams.h>
#include <winrt/Windows.Graphics.h>
#include <winrt/Windows.Graphics.Imaging.h>

#include <Windows.AI.MachineLearning.Native.h>

#include <algorithm>
#include <codecvt>
#include <string>
#include <fcntl.h>
#include <fstream>
#include <io.h>
#include <locale>
#include <numeric>
#include <random>
#include <string_view>
#include <vector>
#include <utility>

using convert_type = std::codecvt_utf8<wchar_t>;
using wstring_to_utf8 = std::wstring_convert<convert_type, wchar_t>;
