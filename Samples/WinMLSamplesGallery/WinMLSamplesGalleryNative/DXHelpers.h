#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN    // Exclude rarely-used stuff from Windows headers.
#endif

#include <windows.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <D3Dcompiler.h>
#include <DirectXMath.h>
#include "d3dx12.h"
#include <string>
#include <wincodec.h>

// create and show the window
bool InitializeWindow(HINSTANCE &hInstance,
    int ShowWnd,
    bool fullscreen,
    HWND &hwnd,
    int Width,
    int Height,
    LPCTSTR WindowName,
    LPCTSTR WindowTitle);

// callback function for windows messages
LRESULT CALLBACK WndProc(HWND hWnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam);