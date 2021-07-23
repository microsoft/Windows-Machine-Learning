#pragma once

#include <unknwn.h>
#include "winrt/Windows.Foundation.h"
#ifdef USE_WINML_NUGET
#include "winrt/Microsoft.AI.MachineLearning.h"
#else
#include "winrt/Windows.AI.MachineLearning.h"
#endif
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Graphics.Imaging.h>
#include <winrt/Windows.Media.h>
#include <winrt/Windows.Storage.h>
#include "winrt/Windows.Storage.Streams.h"

#include <string>
#include <fstream>

#include <Windows.h>