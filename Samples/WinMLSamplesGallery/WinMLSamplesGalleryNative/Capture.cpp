// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
#include "pch.h"
#include "Capture.h"
#include "resource.h"
#include <winrt/Windows.Foundation.h>
#include <winrt/base.h>
#include <mfreadwrite.h>

com_ptr<IMFDXGIDeviceManager> g_DXGIMan;
com_ptr<ID3D11Device>         g_DX11Device;
UINT                          g_ResetToken = 0;


STDMETHODIMP CaptureManager::CaptureEngineCB::QueryInterface(REFIID riid, void** ppv)
{
    static const QITAB qit[] =
    {
        QITABENT(CaptureEngineCB, IMFCaptureEngineOnEventCallback),
        { 0 }
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CaptureManager::CaptureEngineCB::AddRef()
{
    return InterlockedIncrement(&m_ref);
}

STDMETHODIMP_(ULONG) CaptureManager::CaptureEngineCB::Release()
{
    LONG cRef = InterlockedDecrement(&m_ref);
    if (cRef == 0)
    {
        delete this;
    }
    return cRef;
}

// Callback method to receive events from the capture engine.
STDMETHODIMP CaptureManager::CaptureEngineCB::OnEvent(_In_ IMFMediaEvent* pEvent)
{
    // Post a message to the application window, so the event is handled 
    // on the application's main thread. 

    if (m_sleeping && m_captureManager != NULL)
    {
        // We're about to fall asleep, that means we've just asked the CE to stop the preview
        // and record.  We need to handle it here since our message pump may be gone.
        GUID    guidType;
        HRESULT hrStatus;
        RETURN_IF_FAILED(pEvent->GetStatus(&hrStatus));

        RETURN_IF_FAILED(pEvent->GetExtendedType(&guidType));

        if (guidType == MF_CAPTURE_ENGINE_PREVIEW_STOPPED)
        {
            m_captureManager->OnPreviewStopped(hrStatus);
            SetEvent(m_captureManager->m_event);
        }
        else
        {
            // This is an event we don't know about, we don't really care and there's
            // no clean way to report the error so just set the event and fall through.
            SetEvent(m_captureManager->m_event);
        }
    }
    else
    {
        pEvent->AddRef();  // The application will release the pointer when it handles the message.
        PostMessage(m_hwnd, WM_APP_CAPTURE_EVENT, (WPARAM)pEvent, 0L);
    }

    return S_OK;
}

HRESULT CreateDX11Device(_Out_ ID3D11Device** ppDevice, _Out_ ID3D11DeviceContext** ppDeviceContext, _Out_ D3D_FEATURE_LEVEL* pFeatureLevel)
{
    static const D3D_FEATURE_LEVEL levels[] = {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
        D3D_FEATURE_LEVEL_9_3,
        D3D_FEATURE_LEVEL_9_2,
        D3D_FEATURE_LEVEL_9_1
    };

    // Find the adapter with the most video memory
    UINT i = 0;
    com_ptr<IDXGIAdapter> bestAdapater;
    com_ptr<IDXGIAdapter> adapter;
    com_ptr<IDXGIFactory> factory;
    DXGI_ADAPTER_DESC desc;
    size_t maxVideoMem = 0;

    RETURN_IF_FAILED(CreateDXGIFactory1(IID_PPV_ARGS(factory.put())));
    while (factory->EnumAdapters(i, adapter.put()) != DXGI_ERROR_NOT_FOUND)
    {
        adapter->GetDesc(&desc);
        if (desc.DedicatedVideoMemory > maxVideoMem)
        {
            adapter.copy_to(bestAdapater.put());
            maxVideoMem = desc.DedicatedVideoMemory;
        }
        adapter = nullptr;
        ++i;
    }

    RETURN_IF_FAILED(D3D11CreateDevice(
        bestAdapater.get(),
        D3D_DRIVER_TYPE_UNKNOWN,
        nullptr,
        D3D11_CREATE_DEVICE_VIDEO_SUPPORT,
        levels,
        ARRAYSIZE(levels),
        D3D11_SDK_VERSION,
        ppDevice,
        pFeatureLevel,
        ppDeviceContext
    ));

    com_ptr<ID3D10Multithread> pMultithread;
    RETURN_IF_FAILED((*ppDevice)->QueryInterface(IID_PPV_ARGS(pMultithread.put())));
    pMultithread->SetMultithreadProtected(TRUE);

    return S_OK;
}

HRESULT CreateD3DManager()
{
    D3D_FEATURE_LEVEL FeatureLevel;
    com_ptr<ID3D11DeviceContext> DX11DeviceContext;

    RETURN_IF_FAILED(CreateDX11Device(g_DX11Device.put(), DX11DeviceContext.put(), &FeatureLevel));
    RETURN_IF_FAILED(MFCreateDXGIDeviceManager(&g_ResetToken, g_DXGIMan.put()));
    RETURN_IF_FAILED(g_DXGIMan->ResetDevice(g_DX11Device.get(), g_ResetToken));

    return S_OK;
}

// https://github.com/Microsoft/wil/wiki/Error-handling-helpers#using-exception-based-code-in-a-routine-that-cannot-throw
// Use function guard and don't co-mix exception and error handling in a single function
HRESULT CaptureManager::InitializeCaptureManager(HWND hwndPreview, HWND hwndStatus, IUnknown* pUnk) noexcept try
{
    HRESULT                         hr = S_OK;
    com_ptr<IMFAttributes>          attributes;
    com_ptr<IMFCaptureEngineClassFactory>   factory;

    DestroyCaptureEngine();

    m_event = CreateEvent(NULL, FALSE, FALSE, NULL);
    THROW_LAST_ERROR_IF_NULL(m_event);

    m_callback.attach(new CaptureEngineCB(m_hwndEvent));

    m_callback->m_captureManager = this;
    m_hwndPreview = hwndPreview;
    m_hwndStatus = hwndStatus;

    //Create a D3D Manager
    THROW_IF_FAILED(CreateD3DManager());

    THROW_IF_FAILED(MFCreateAttributes(attributes.put(), 1));

    THROW_IF_FAILED(attributes->SetUnknown(MF_CAPTURE_ENGINE_D3D_MANAGER, g_DXGIMan.get()));

    // Create the factory object for the capture engine.
    THROW_IF_FAILED(CoCreateInstance(CLSID_MFCaptureEngineClassFactory, NULL,
        CLSCTX_INPROC_SERVER, IID_PPV_ARGS(factory.put())));

    // Create and initialize the capture engine.
    THROW_IF_FAILED(factory->CreateInstance(CLSID_MFCaptureEngine, IID_PPV_ARGS(m_engine.put())));

    THROW_IF_FAILED(m_engine->Initialize(m_callback.get(), attributes.get(), NULL, pUnk));
}
CATCH_RETURN();

// Handle an event from the capture engine. 
// NOTE: This method is called from the application's UI thread. 
HRESULT CaptureManager::OnCaptureEvent(WPARAM wParam, LPARAM lParam)
{
    GUID guidType;
    HRESULT hrStatus = S_OK;

    com_ptr<IMFMediaEvent> event;
    event.copy_from(reinterpret_cast<IMFMediaEvent*>(wParam));

    hrStatus = event->GetStatus(&hrStatus);

    RETURN_IF_FAILED(event->GetExtendedType(&guidType));

#ifdef _DEBUG
    LPOLESTR str;
    if (SUCCEEDED(StringFromCLSID(guidType, &str)))
    {
        //TRACE((L"MF_CAPTURE_ENGINE_EVENT: %s (hr = 0x%X)\n", str, hrStatus));
        CoTaskMemFree(str);
    }
#endif

    if (guidType == MF_CAPTURE_ENGINE_INITIALIZED)
    {
        OnCaptureEngineInitialized(hrStatus);
        SetErrorID(hrStatus, IDS_ERR_INITIALIZE);
    }
    else if (guidType == MF_CAPTURE_ENGINE_PREVIEW_STARTED)
    {
        OnPreviewStarted(hrStatus);
        SetErrorID(hrStatus, IDS_ERR_PREVIEW);
    }
    else if (guidType == MF_CAPTURE_ENGINE_PREVIEW_STOPPED)
    {
        OnPreviewStopped(hrStatus);
        SetErrorID(hrStatus, IDS_ERR_PREVIEW);
    }
    else if (guidType == MF_CAPTURE_ENGINE_ERROR)
    {
        DestroyCaptureEngine();
        SetErrorID(hrStatus, IDS_ERR_CAPTURE);
    }
    else if (FAILED(hrStatus))
    {
        SetErrorID(hrStatus, IDS_ERR_CAPTURE);
    }

    SetEvent(m_event);
    return hrStatus;
}


void CaptureManager::OnCaptureEngineInitialized(HRESULT& hrStatus)
{
    if (hrStatus == MF_E_NO_CAPTURE_DEVICES_AVAILABLE)
    {
        hrStatus = S_OK;  // No capture device. Not an application error.
    }
}

void CaptureManager::OnPreviewStarted(HRESULT& hrStatus)
{
    m_previewing = SUCCEEDED(hrStatus);
}

void CaptureManager::OnPreviewStopped(HRESULT& hrStatus)
{
    m_previewing = false;
}

/*
* Begins a preview stream by initializing the preview sink, adding a TransformAsync
* MFT to the preview stream, and signalling the capture engine to begin previewing frames.
*/
HRESULT CaptureManager::StartPreview(winrt::hstring modelPath)
{
    if (m_engine == NULL)
    {
        return MF_E_NOT_INITIALIZED;
    }
    if (m_previewing == true)
    {
        return S_OK;
    }
    com_ptr<IMFCaptureSink> sink;
    com_ptr<IMFMediaType> mediaType;
    com_ptr<IMFMediaType> mediaType2;
    com_ptr<IMFCaptureSource> source;

    // Get a pointer to the preview sink.
    if (m_preview == NULL)
    {
        RETURN_IF_FAILED(m_engine->GetSink(MF_CAPTURE_ENGINE_SINK_TYPE_PREVIEW, sink.put()));
        RETURN_IF_FAILED(sink->QueryInterface(IID_PPV_ARGS(m_preview.put())));
        RETURN_IF_FAILED(m_preview->SetRenderHandle(m_hwndPreview));
        RETURN_IF_FAILED(m_engine->GetSource(source.put()));

        // Configure the video format for the preview sink.
        RETURN_IF_FAILED(source->GetCurrentDeviceMediaType((DWORD)MF_CAPTURE_ENGINE_PREFERRED_SOURCE_STREAM_FOR_VIDEO_PREVIEW, mediaType.put()));

        // Add the transform s
        com_ptr<IMFTransform> mft;
        RETURN_IF_FAILED(TransformAsync::CreateInstance(mft.put()));
        mft.as<TransformAsync>()->SetFrameRateWnd(m_hwndStatus);
        mft.as<TransformAsync>()->SetModelBasePath(modelPath);

        // IMFCaptureSource
        RETURN_IF_FAILED(source->AddEffect(0, mft.get()));
        RETURN_IF_FAILED(CloneVideoMediaType(mediaType.get(), MFVideoFormat_RGB32, mediaType2.put()));
        RETURN_IF_FAILED(mediaType2->SetUINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, TRUE));

        // Connect the video stream to the preview sink.
        DWORD dwSinkStreamIndex;
        RETURN_IF_FAILED(m_preview->AddStream((DWORD)MF_CAPTURE_ENGINE_PREFERRED_SOURCE_STREAM_FOR_VIDEO_PREVIEW, mediaType2.get(), NULL, &dwSinkStreamIndex));
    }

    RETURN_IF_FAILED(m_engine->StartPreview());
    if (!m_fPowerRequestSet && m_hpwrRequest != INVALID_HANDLE_VALUE)
    {
        // NOTE:  By calling this, on SOC systems (AOAC enabled), we're asking the system to not go
        // into sleep/connected standby while we're streaming.  However, since we don't want to block
        // the device from ever entering connected standby/sleep, we're going to latch ourselves to
        // the monitor on/off notification (RegisterPowerSettingNotification(GUID_MONITOR_POWER_ON)).
        // On SOC systems, this notification will fire when the user decides to put the device in
        // connected standby mode--we can trap this, turn off our media streams and clear this
        // power set request to allow the device to go into the lower power state.
        m_fPowerRequestSet = (TRUE == PowerSetRequest(m_hpwrRequest, PowerRequestExecutionRequired));
    }

    return S_OK;
}

HRESULT CaptureManager::StopPreview()
{
    if (m_engine == NULL)
    {
        return MF_E_NOT_INITIALIZED;
    }

    if (!m_previewing)
    {
        return S_OK;
    }

    RETURN_IF_FAILED_WITH_EXPECTED(m_engine->StopPreview(), MF_E_INVALIDREQUEST);
    WaitForResult();

    return S_OK;
}


// Helper function to get the frame size from a video media type.
inline HRESULT GetFrameSize(IMFMediaType* pType, UINT32* pWidth, UINT32* pHeight)
{
    return MFGetAttributeSize(pType, MF_MT_FRAME_SIZE, pWidth, pHeight);
}

// Helper function to get the frame rate from a video media type.
inline HRESULT GetFrameRate(
    IMFMediaType* pType,
    UINT32* pNumerator,
    UINT32* pDenominator
)
{
    return MFGetAttributeRatio(
        pType,
        MF_MT_FRAME_RATE,
        pNumerator,
        pDenominator
    );
}


HRESULT GetEncodingBitrate(IMFMediaType* pMediaType, UINT32* uiEncodingBitrate)
{
    UINT32 width;
    UINT32 height;
    float bitrate;
    UINT32 frameRateNum;
    UINT32 frameRateDenom;

    RETURN_IF_FAILED(GetFrameSize(pMediaType, &width, &height));
    RETURN_IF_FAILED(GetFrameRate(pMediaType, &frameRateNum, &frameRateDenom));
    bitrate = width / 3.0f * height * frameRateNum / frameRateDenom;
    *uiEncodingBitrate = (UINT32)bitrate;
    return S_OK;
}



