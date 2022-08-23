#pragma once
#include "D3D12Quad.h"

class Win32Application
{
public:
    static int Run(D3D12Quad* pSample, int nCmdShow);
    static HWND GetHwnd() { return m_hwnd; }
    static void CloseWindow();
    static std::wstring GetAssetPath(LPCWSTR assetName);

protected:
    static LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

private:
    static HWND m_hwnd;
    static bool m_closeWindow;
};