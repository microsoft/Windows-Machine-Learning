// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.

#ifndef CAPTURE_H
#define CAPTURE_H

#ifndef UNICODE
#define UNICODE
#endif 

#if !defined( NTDDI_VERSION )
#define NTDDI_VERSION NTDDI_WIN8
#endif

#if !defined( _WIN32_WINNT )
#define _WIN32_WINNT _WIN32_WINNT_WIN8
#endif

#include <new>
#include <windows.h>
#include <windowsx.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mferror.h>
#include <mfcaptureengine.h>
#include <shlwapi.h>
#include <strsafe.h>
#include <commctrl.h>
#include <d3d11.h>
#include <initguid.h>
//#include "Helpers/common.h"
#include "TransformAsync.h"

const UINT WM_APP_CAPTURE_EVENT = WM_APP + 1;

HWND    CreatePreviewWindow(HINSTANCE hInstance, HWND hParent);
HWND    CreateMainWindow(HINSTANCE hInstance);
void    SetMenuItemText(HMENU hMenu, UINT uItem, _In_ PWSTR pszText);
void    ShowError(HWND hwnd, PCWSTR szMessage, HRESULT hr);
void    ShowError(HWND hwnd, UINT id, HRESULT hr);
HRESULT CloneVideoMediaType(IMFMediaType* pSrcMediaType, REFGUID guidSubType, IMFMediaType** ppNewMediaType);

// DXGI DevManager support
extern com_ptr<IMFDXGIDeviceManager> g_DXGIMan;
extern com_ptr<ID3D11Device>         g_DX11Device;
extern UINT                  g_ResetToken;

// Gets an interface pointer from a Media Foundation collection.
template <class IFACE>
HRESULT GetCollectionObject(IMFCollection* pCollection, DWORD index, IFACE** ppObject)
{
    IUnknown* unk;
    HRESULT hr = pCollection->GetElement(index, &unk);
    if (SUCCEEDED(hr))
    {
        hr = unk->QueryInterface(IID_PPV_ARGS(ppObject));
        unk->Release();
    }
    return hr;
}


struct ChooseDeviceParam
{
    ChooseDeviceParam() : devices(NULL), count(0)
    {
    }
    ~ChooseDeviceParam()
    {
        for (DWORD i = 0; i < count; i++)
        {
            SAFE_RELEASE(devices[i]);
        }
        CoTaskMemFree(devices);

    }

    IMFActivate** devices;
    UINT32      count;
    UINT32      selection;
};



// CaptureManager class
// Wraps the capture engine and implements the event callback.

class CaptureManager
{
    // The event callback object.
    class CaptureEngineCB : public IMFCaptureEngineOnEventCallback
    {
        long m_ref;
        HWND m_hwnd;

    public:
        CaptureEngineCB(HWND hwnd) : m_ref(1), m_hwnd(hwnd), m_sleeping(false), m_captureManager(NULL) {}

        // IUnknown
        STDMETHODIMP QueryInterface(REFIID riid, void** ppv);
        STDMETHODIMP_(ULONG) AddRef();
        STDMETHODIMP_(ULONG) Release();

        // IMFCaptureEngineOnEventCallback
        STDMETHODIMP OnEvent(_In_ IMFMediaEvent* pEvent);

        bool m_sleeping;
        CaptureManager* m_captureManager;
    };

    HWND                    m_hwndEvent;    // Event message thread for responding to MF
    HWND                    m_hwndPreview;  // Preview window to render frames 
    HWND                    m_hwndStatus;   // For displaying status messages, eg. frame rate

    com_ptr<IMFCaptureEngine>       m_engine;        // Manages the capture engine (ie. the camera) 
    com_ptr<IMFCapturePreviewSink>  m_preview;  // Manages the preview sink (ie. the video window) 

    com_ptr<CaptureEngineCB> m_callback;

    bool                    m_previewing;

    UINT                    m_errorID;
    HANDLE                  m_event;
    HANDLE                  m_hpwrRequest; // TODO: REMOVE
    bool                    m_fPowerRequestSet;

    CaptureManager(HWND hwnd) :
        m_hwndEvent(hwnd), m_hwndPreview(NULL), m_engine(NULL), m_preview(NULL),
        m_callback(NULL), m_previewing(false), m_errorID(0), m_event(NULL)
        , m_hpwrRequest(INVALID_HANDLE_VALUE)
        , m_fPowerRequestSet(false)
    {
        REASON_CONTEXT  pwrCtxt;

        pwrCtxt.Version = POWER_REQUEST_CONTEXT_VERSION;
        pwrCtxt.Flags = POWER_REQUEST_CONTEXT_SIMPLE_STRING;
        pwrCtxt.Reason.SimpleReasonString = L"CaptureEngine is recording!";

        m_hpwrRequest = PowerCreateRequest(&pwrCtxt);
    }

    void SetErrorID(HRESULT hr, UINT id)
    {
        m_errorID = SUCCEEDED(hr) ? 0 : id;
    }

    // Capture Engine Event Handlers
    void OnCaptureEngineInitialized(HRESULT& hrStatus);
    void OnPreviewStarted(HRESULT& hrStatus);
    void OnPreviewStopped(HRESULT& hrStatus);
    void WaitForResult()
    {
        WaitForSingleObject(m_event, INFINITE);
    }
public:
    ~CaptureManager()
    {
        DestroyCaptureEngine();
    }

    static HRESULT CreateInstance(HWND hwndEvent, CaptureManager** ppEngine) noexcept try
    {
        *ppEngine = NULL;

        CaptureManager* engine = new CaptureManager(hwndEvent);
        *ppEngine = engine;
        engine = NULL;

        return S_OK;
    }CATCH_RETURN();

    HRESULT InitializeCaptureManager(HWND hwndPreview, HWND hwndMessage, IUnknown* pUnk) noexcept;
    void DestroyCaptureEngine()
    {
        if (NULL != m_event)
        {
            CloseHandle(m_event);
            m_event = NULL;
        }

        if (g_DXGIMan)
        {
            g_DXGIMan->ResetDevice(g_DX11Device.get(), g_ResetToken);
        }
        
        if (m_previewing)
        {
            com_ptr<IMFCaptureSource> source;
            m_engine->GetSource(source.put());
            source->RemoveAllEffects(0);
            m_engine->StopPreview();
        }

        m_previewing = false;
        m_errorID = 0;
    }



    bool    IsPreviewing() const { return m_previewing; }
    UINT    ErrorID() const { return m_errorID; }

    HRESULT OnCaptureEvent(WPARAM wParam, LPARAM lParam);
    HRESULT StartPreview(winrt::hstring modelPath);
    HRESULT StopPreview();

    void    SleepState(bool fSleeping)
    {
        if (NULL != m_callback)
        {
            m_callback->m_sleeping = fSleeping;
        }
    }

    HRESULT UpdateVideo()
    {
        if (m_preview)
        {
            return m_preview->UpdateVideo(NULL, NULL, NULL);
        }
        else
        {
            return S_OK;
        }
    }
};

#endif CAPTURE_H
