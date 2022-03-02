#include "pch.h"
#include "PixRedistMiddleware.h"

void TryEnsurePIXFunctions()
{
    std::lock_guard lock(g_mutex);
    if (g_pixLoadAttempted == false) 
    {
        g_pixLoadAttempted = true;
        std::vector<wchar_t> modulePath(MAX_PATH);
        while (true)
        {
            DWORD moduleLength = GetModuleFileNameW(nullptr, modulePath.data(), static_cast<DWORD>(modulePath.size()));
            if (moduleLength == 0)
            {
                return;
            }
            if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
            {
                modulePath.resize(modulePath.size() * 2);
            }
            else 
            {
                break;
            }
        }
            
        std::filesystem::path dllPath(modulePath.data());
        uint32_t length = 0;
        long rc = GetCurrentPackageFullName(&length, nullptr);
        // APPMODEL_ERROR_NO_PACKAGE will be returned if running in desktop context
        // see: https://docs.microsoft.com/en-us/archive/blogs/appconsult/desktop-bridge-identify-the-applications-context
        if (rc == APPMODEL_ERROR_NO_PACKAGE)
        {
            dllPath.replace_filename(L"WinPixEventRuntime.dll");
        }
        else
        {
            dllPath.replace_filename(L"WinPixEventRuntime_UAP.dll");
        }

        // Intentionally leaked
        HMODULE pixRuntime = LoadLibrary(dllPath.c_str());   
        HMODULE pixGPURuntime = LoadLibrary(L"C:\\Program Files\\Microsoft PIX\\2202.25-GpuTimingDataFix\\WinPixGpuCapturer.dll");
        if (pixRuntime != nullptr)
        {
            g_beginCaptureSingleton = (BeginCapture)GetProcAddress(pixRuntime, "PIXBeginCapture2");
            g_endCaptureSingleton = (EndCapture)GetProcAddress(pixRuntime, "PIXEndCapture");
            g_beginEventSingleton = (BeginEventOnCommandQueue)GetProcAddress(pixRuntime, "PIXBeginEventOnCommandQueue");
            g_endEventSingleton = (EndEventOnCommandQueue)GetProcAddress(pixRuntime, "PIXEndEventOnCommandQueue");
			g_setMarkerSingleton = (SetMarkerOnCommandQueue)GetProcAddress(pixRuntime, "PIXSetMarkerOnCommandQueue");
        }
    }
}

void PIXBeginEvent(ID3D12CommandQueue* commandQueue, UINT64 color, _In_ PCSTR string)
{    
    if (g_beginEventSingleton == nullptr)
    {
        // Silently No-Op (PIX runtime was not resolved in module directory)
        return;
    }
    g_beginEventSingleton(commandQueue, color, string);
}

void PIXEndEvent(ID3D12CommandQueue* commandQueue)
{
    if (g_endEventSingleton == nullptr)
    {
        // Silently No-Op (PIX runtime was not resolved in module directory)
        return;
    }
    g_endEventSingleton(commandQueue);
}

void PIXSetMarker(ID3D12CommandQueue* commandQueue, UINT64 color, _In_ PCSTR string)
{    
    if (g_setMarkerSingleton == nullptr)
    {
        // Silently No-Op (PIX runtime was not resolved in module directory)
        return;
    }
    g_setMarkerSingleton(commandQueue, color, string);
}

void PIXBeginCaptureRedist(DWORD captureFlags, _In_ PPIXCaptureParameters captureParameters)
{
    if (g_beginCaptureSingleton == nullptr)
    {
        // Silently No-Op (PIX runtime was not resolved in module directory)
        return;
    }
    g_beginCaptureSingleton(captureFlags, captureParameters);
}

void PIXEndCaptureRedist(BOOL discard)
{
    if (g_endCaptureSingleton == nullptr)
    {
        // Silently No-Op (PIX runtime was not resolved in module directory)
        return;
    }
    g_endCaptureSingleton(discard);
}
