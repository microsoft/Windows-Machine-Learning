// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.

#include "Capture.h"
#include "resource.h"
#include "TransformAsync.h"
#include <winrt/Windows.Foundation.h>
#include <winrt/base.h>
#include <mfreadwrite.h>

com_ptr<IMFDXGIDeviceManager> g_pDXGIMan;
com_ptr<ID3D11Device>         g_pDX11Device;
UINT                  g_ResetToken = 0;


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
    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) CaptureManager::CaptureEngineCB::Release()
{
    LONG cRef = InterlockedDecrement(&m_cRef);
    if (cRef == 0)
    {
        delete this;
    }
    return cRef;
}

// Callback method to receive events from the capture engine.
STDMETHODIMP CaptureManager::CaptureEngineCB::OnEvent( _In_ IMFMediaEvent* pEvent)
{
    // Post a message to the application window, so the event is handled 
    // on the application's main thread. 

    if (m_fSleeping && m_pManager != NULL)
    {
        // We're about to fall asleep, that means we've just asked the CE to stop the preview
        // and record.  We need to handle it here since our message pump may be gone.
        GUID    guidType;
        HRESULT hrStatus;
        RETURN_IF_FAILED(pEvent->GetStatus(&hrStatus));

        RETURN_IF_FAILED(pEvent->GetExtendedType(&guidType));

        if (guidType == MF_CAPTURE_ENGINE_PREVIEW_STOPPED)
        {
            m_pManager->OnPreviewStopped(hrStatus);
            SetEvent(m_pManager->m_hEvent);
        }
        else
        {
            // This is an event we don't know about, we don't really care and there's
            // no clean way to report the error so just set the event and fall through.
            SetEvent(m_pManager->m_hEvent);
        }
    }
    else
    {
        pEvent->AddRef();  // The application will release the pointer when it handles the message.
        PostMessage(m_hwnd, WM_APP_CAPTURE_EVENT, (WPARAM)pEvent, 0L);
    }

    return S_OK;
}

HRESULT CreateDX11Device(_Out_ ID3D11Device** ppDevice, _Out_ ID3D11DeviceContext** ppDeviceContext, _Out_ D3D_FEATURE_LEVEL* pFeatureLevel )
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
    com_ptr<IDXGIAdapter> pBestAdapter;
    com_ptr<IDXGIAdapter> spAdapter;
    com_ptr<IDXGIFactory> spFactory;
    DXGI_ADAPTER_DESC desc;
    size_t maxVideoMem = 0;

    RETURN_IF_FAILED(CreateDXGIFactory1(IID_PPV_ARGS(spFactory.put())));
    while (spFactory->EnumAdapters(i, spAdapter.put()) != DXGI_ERROR_NOT_FOUND) 
    {
        spAdapter->GetDesc(&desc);
        if (desc.DedicatedVideoMemory > maxVideoMem)
        {
            spAdapter.copy_to(pBestAdapter.put());
            maxVideoMem = desc.DedicatedVideoMemory;
        }
        spAdapter = nullptr;
        ++i;
    }

    RETURN_IF_FAILED(D3D11CreateDevice(
        pBestAdapter.get(),
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
    com_ptr<ID3D11DeviceContext> pDX11DeviceContext;
    
    RETURN_IF_FAILED(CreateDX11Device(g_pDX11Device.put(), pDX11DeviceContext.put(), &FeatureLevel));
    RETURN_IF_FAILED(MFCreateDXGIDeviceManager(&g_ResetToken, g_pDXGIMan.put()));
    RETURN_IF_FAILED(g_pDXGIMan->ResetDevice(g_pDX11Device.get(), g_ResetToken));
        
    return S_OK;
}

// https://github.com/Microsoft/wil/wiki/Error-handling-helpers#using-exception-based-code-in-a-routine-that-cannot-throw
// Use function guard and don't co-mix exception and error handling in a single function
HRESULT CaptureManager::InitializeCaptureManager(HWND hwndPreview, IUnknown* pUnk) noexcept try
{
    HRESULT                         hr = S_OK;
    com_ptr<IMFAttributes>          pAttributes;
    com_ptr<IMFCaptureEngineClassFactory>   pFactory;

    DestroyCaptureEngine();

    m_hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    THROW_LAST_ERROR_IF_NULL(m_hEvent);

    m_pCallback.attach(new CaptureEngineCB(m_hwndEvent));

    m_pCallback->m_pManager = this;
    m_hwndPreview = hwndPreview;

    //Create a D3D Manager
    THROW_IF_FAILED(CreateD3DManager());

    THROW_IF_FAILED(MFCreateAttributes(pAttributes.put(), 1));

    THROW_IF_FAILED(pAttributes->SetUnknown(MF_CAPTURE_ENGINE_D3D_MANAGER, g_pDXGIMan.get()));

    // Create the factory object for the capture engine.
    THROW_IF_FAILED(CoCreateInstance(CLSID_MFCaptureEngineClassFactory, NULL,
        CLSCTX_INPROC_SERVER, IID_PPV_ARGS(pFactory.put())));

    // Create and initialize the capture engine.
    THROW_IF_FAILED(pFactory->CreateInstance(CLSID_MFCaptureEngine, IID_PPV_ARGS(m_pEngine.put())));

    THROW_IF_FAILED(m_pEngine->Initialize(m_pCallback.get(), pAttributes.get(), NULL, pUnk));
}
CATCH_RETURN();

// Handle an event from the capture engine. 
// NOTE: This method is called from the application's UI thread. 
HRESULT CaptureManager::OnCaptureEvent(WPARAM wParam, LPARAM lParam)
{
    GUID guidType;
    HRESULT hrStatus = S_OK;

    com_ptr<IMFMediaEvent> pEvent;
    pEvent.copy_from(reinterpret_cast<IMFMediaEvent*>(wParam));

    hrStatus = pEvent->GetStatus(&hrStatus);

    RETURN_IF_FAILED(pEvent->GetExtendedType(&guidType));

#ifdef _DEBUG
    LPOLESTR str;
    if (SUCCEEDED(StringFromCLSID(guidType, &str)))
    {
        TRACE((L"MF_CAPTURE_ENGINE_EVENT: %s (hr = 0x%X)\n", str, hrStatus));
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

    SetEvent(m_hEvent);
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
    m_bPreviewing = SUCCEEDED(hrStatus);
}

void CaptureManager::OnPreviewStopped(HRESULT& hrStatus)
{
    m_bPreviewing = false;
}

/*
* Begins a preview stream by initializing the preview sink, adding a TransformAsync
* MFT to the preview stream, and signalling the capture engine to begin previewing frames.
*/
HRESULT CaptureManager::StartPreview()
{
    if (m_pEngine == NULL)
    {
        return MF_E_NOT_INITIALIZED;
    }

    if (m_bPreviewing == true)
    {
        return S_OK;
    }

    com_ptr<IMFCaptureSink> pSink;
    com_ptr<IMFMediaType> pMediaType;
    com_ptr<IMFMediaType> pMediaType2;
    com_ptr<IMFCaptureSource> pSource;

    
    // Get a pointer to the preview sink.
    if (m_pPreview == NULL)
    {
        RETURN_IF_FAILED(m_pEngine->GetSink(MF_CAPTURE_ENGINE_SINK_TYPE_PREVIEW, pSink.put()));
        RETURN_IF_FAILED(pSink->QueryInterface(IID_PPV_ARGS(m_pPreview.put())));
        RETURN_IF_FAILED(m_pPreview->SetRenderHandle(m_hwndPreview));
        RETURN_IF_FAILED(m_pEngine->GetSource(pSource.put()));

        // Configure the video format for the preview sink.
        RETURN_IF_FAILED(pSource->GetCurrentDeviceMediaType((DWORD)MF_CAPTURE_ENGINE_PREFERRED_SOURCE_STREAM_FOR_VIDEO_PREVIEW , pMediaType.put()));

        // Add the transform 
        com_ptr<IMFTransform>pMFT;
        RETURN_IF_FAILED(TransformAsync::CreateInstance(pMFT.put()));

        // IMFCaptureSource
        RETURN_IF_FAILED(pSource->AddEffect(0, pMFT.get()));
        RETURN_IF_FAILED(CloneVideoMediaType(pMediaType.get(), MFVideoFormat_RGB32, pMediaType2.put()));
        RETURN_IF_FAILED(pMediaType2->SetUINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, TRUE));

        // Connect the video stream to the preview sink.
        DWORD dwSinkStreamIndex;
        RETURN_IF_FAILED(m_pPreview->AddStream((DWORD)MF_CAPTURE_ENGINE_PREFERRED_SOURCE_STREAM_FOR_VIDEO_PREVIEW,  pMediaType2.get(), NULL, &dwSinkStreamIndex));
    }


    RETURN_IF_FAILED(m_pEngine->StartPreview());
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
    if (m_pEngine == NULL)
    {
        return MF_E_NOT_INITIALIZED;
    }

    if (!m_bPreviewing)
    {
        return S_OK;
    }

    RETURN_IF_FAILED_WITH_EXPECTED(m_pEngine->StopPreview(), MF_E_INVALIDREQUEST);
    WaitForResult();

    if (m_fPowerRequestSet && m_hpwrRequest != INVALID_HANDLE_VALUE)
    {
        PowerClearRequest(m_hpwrRequest, PowerRequestExecutionRequired);
        m_fPowerRequestSet = false;
    }

    return S_OK;
}


// Helper function to get the frame size from a video media type.
inline HRESULT GetFrameSize(IMFMediaType *pType, UINT32 *pWidth, UINT32 *pHeight)
{    return MFGetAttributeSize(pType, MF_MT_FRAME_SIZE, pWidth, pHeight);}

// Helper function to get the frame rate from a video media type.
inline HRESULT GetFrameRate(
    IMFMediaType *pType, 
    UINT32 *pNumerator, 
    UINT32 *pDenominator
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
    UINT32 uiWidth;
    UINT32 uiHeight;
    float uiBitrate;
    UINT32 uiFrameRateNum;
    UINT32 uiFrameRateDenom;

    RETURN_IF_FAILED(GetFrameSize(pMediaType, &uiWidth, &uiHeight));

    RETURN_IF_FAILED(GetFrameRate(pMediaType, &uiFrameRateNum, &uiFrameRateDenom));

    uiBitrate = uiWidth / 3.0f * uiHeight * uiFrameRateNum / uiFrameRateDenom;

    *uiEncodingBitrate = (UINT32)uiBitrate;

    return S_OK;
}



