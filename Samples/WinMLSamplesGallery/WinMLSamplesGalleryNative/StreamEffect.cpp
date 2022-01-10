#include "pch.h"
#include "StreamEffect.h"
#include "StreamEffect.g.cpp"

#include <commctrl.h>
#include <mfapi.h>


namespace winrt::WinMLSamplesGalleryNative::implementation
{
    LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        switch (uMsg) {
        default:
            return 1;
        }
    };

    void StreamEffect::LaunchNewWindow()
    {
        HWND hwnd;
        HRESULT hr = S_OK;
        BOOL bMFStartup = false;
        
        // Initialize the common controls
        const INITCOMMONCONTROLSEX icex = { sizeof(INITCOMMONCONTROLSEX), ICC_WIN95_CLASSES };
        InitCommonControlsEx(&icex);

        hr = MFStartup(MF_VERSION);
        if (FAILED(hr))
        {
            goto done;
        }

        // Create window
        const wchar_t CLASS_NAME[] = L"Capture Engine Window Class";
        INT nCmdShow = 1;

        WNDCLASS wc = { 0 };

        wc.lpfnWndProc = WindowProc;
        wc.hInstance = GetModuleHandle(NULL);
        wc.lpszClassName = CLASS_NAME;

        RegisterClass(&wc);

        hwnd = CreateWindowEx(
            0, CLASS_NAME, L"Capture Application", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
            CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, GetModuleHandle(NULL), NULL
        );

        if (hwnd == 0)
        {
            throw_hresult(E_FAIL);
        }

        ShowWindow(hwnd, nCmdShow);

        // Run the main message loop
        MSG msg;
        while (GetMessage(&msg, NULL, 0, 0))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

    done: 
        if (FAILED(hr))
        {
            // TODO: Add utils from BackgroundBlur sample utils.cpp
            //ShowError(NULL, L"Failed to start application", hr);
        }
        if (bMFStartup)
        {
            MFShutdown();
        }
        return ;
    }
}
