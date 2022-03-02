#pragma once
#include "pch.h"
#include <appmodel.h>
#include <mutex>
#include <filesystem>

#ifndef PIXREDIST
#define PIXREDIST

typedef HRESULT(WINAPI* BeginCapture)(DWORD captureFlags, _In_ PPIXCaptureParameters captureParameters);
typedef HRESULT(WINAPI* EndCapture)(BOOL discard);
typedef HRESULT(WINAPI* BeginEventOnCommandQueue)(ID3D12CommandQueue* commandQueue, UINT64 color, _In_ PCSTR formatString);
typedef HRESULT(WINAPI* EndEventOnCommandQueue)(ID3D12CommandQueue* commandQueue);
typedef HRESULT(WINAPI* SetMarkerOnCommandQueue)(ID3D12CommandQueue* commandQueue, UINT64 color, _In_ PCSTR formatString);

static BeginCapture g_beginCaptureSingleton = nullptr;
static EndCapture g_endCaptureSingleton = nullptr;
static BeginEventOnCommandQueue g_beginEventSingleton = nullptr;
static EndEventOnCommandQueue g_endEventSingleton = nullptr;
static SetMarkerOnCommandQueue g_setMarkerSingleton = nullptr;
static bool g_pixLoadAttempted = false;
static std::mutex g_mutex;

void TryEnsurePIXFunctions();

void PIXBeginEvent(ID3D12CommandQueue* commandQueue, UINT64 color, _In_ PCSTR string);

void PIXEndEvent(ID3D12CommandQueue* commandQueue);

void PIXSetMarker(ID3D12CommandQueue* commandQueue, UINT64 color, _In_ PCSTR string);

void PIXBeginCaptureRedist(DWORD captureFlags, _In_ PPIXCaptureParameters captureParameters);

void PIXEndCaptureRedist(BOOL discard);

#endif