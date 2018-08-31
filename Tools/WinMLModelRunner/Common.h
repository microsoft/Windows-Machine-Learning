#pragma once

#define _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS
#include <winrt/Windows.AI.MachineLearning.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Media.h>
#include <winrt/Windows.Graphics.Imaging.h>
#include <winrt/Windows.Media.h>
#include "winrt/Windows.Storage.h"
#include <winrt/Windows.Storage.Streams.h>
#include "TimerHelper.h"
#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include <comdef.h>
#include <algorithm>
#include <numeric>
#include <cassert>
#include <fstream>
#include <dxgi1_6.h>

enum WINML_MODEL_TEST_PERF
{
	ENTIRE_TEST = 0,
	LOAD_MODEL,
	CREATE_SESSION,
	BIND_VALUE,
	EVAL_MODEL,
	EVAL_MODEL_FIRST_RUN,
	COUNT
};

static std::vector<std::wstring> WINML_MODEL_TEST_PERF_NAMES =
{
	L"ENTIRE TEST          ",
	L"  LOAD MODEL         ",
	L"  CREATE SESSION     ",
	L"  BIND VALUE         ",
	L"  EVAL MODEL         ",
};

#define MAX_PROFILING_LOOP 100

using namespace winrt;

inline std::wstring MakeErrorMsg(HRESULT hr)
{
	std::wostringstream ss;
	ss << L"0x" << std::hex << hr << ": " << _com_error(hr).ErrorMessage();
	return ss.str();
}

inline std::wstring MakeErrorMsg(HRESULT hr, const std::wstring &errorMsg)
{
	std::wostringstream ss;
	ss << errorMsg << L" (" << (MakeErrorMsg(hr)) << L")";
	return ss.str();
}

inline void WriteErrorMsg(const std::wstring &errorMsg)
{
	std::wostringstream ss;
	ss << L"ERROR: " << errorMsg << std::endl;
	OutputDebugStringW(ss.str().c_str());
	std::wcout << ss.str() << std::endl;
}

inline void WriteErrorMsg(HRESULT hr, const std::wstring &errorMsg = L"")
{
	std::wostringstream ss;
	ss << errorMsg << L" (" << (MakeErrorMsg(hr)) << L")";
	WriteErrorMsg(ss.str());
}

inline void ThrowIfFailed(HRESULT hr, const std::wstring &errorMsg = L"")
{
	if (FAILED(hr))
	{
		throw MakeErrorMsg(hr, errorMsg);
	}
}

inline void ThrowFailure(const std::wstring &errorMsg)
{
	throw errorMsg;
}