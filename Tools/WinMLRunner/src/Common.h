#pragma once
#ifndef _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS
#define _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS
#endif
#include <vcruntime.h>
#include <windows.h>
// unknown.h needs to be inlcuded before any winrt headers
#include <unknwn.h>
#include <winrt/Windows.AI.MachineLearning.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Media.h>
#include <winrt/Windows.Graphics.Imaging.h>
#include <winrt/Windows.Media.h>
#include <winrt/Windows.Storage.h>
#include <winrt/Windows.Storage.Streams.h>
#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include <comdef.h>
#include <algorithm>
#include <numeric>
#include <cassert>
#include <fstream>
#include <Windows.AI.MachineLearning.Native.h>
#include <dxgi1_6.h>
#include <d3d12.h>
#include "TypeHelper.h"
#include "TimerHelper.h"

extern std::wstring g_loadWinMLDllPath;

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

//
// Delay load exception information
//
#ifndef FACILITY_VISUALCPP
#define FACILITY_VISUALCPP  ((LONG)0x6d)
#endif

#define VcppException(sev,err)  ((sev) | (FACILITY_VISUALCPP<<16) | err)

HRESULT CreateDXGIFactory2SEH(void **pIDXGIFactory);

