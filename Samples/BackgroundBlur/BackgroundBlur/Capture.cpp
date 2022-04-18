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
        HRESULT hr = pEvent->GetStatus(&hrStatus);
        if (FAILED(hr))
        {
            hrStatus = hr;
        }

        hr = pEvent->GetExtendedType(&guidType);
        if (SUCCEEDED(hr))
        {
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

        return S_OK;
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
    HRESULT hr = S_OK;
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

    std::vector <IDXGIAdapter*> vAdapters;
    com_ptr<IDXGIAdapter> pBestAdapter;

    com_ptr<IDXGIAdapter> spAdapter;
    com_ptr<IDXGIFactory> spFactory;
    DXGI_ADAPTER_DESC desc;
    hr = CreateDXGIFactory1(IID_PPV_ARGS(spFactory.put()));
    size_t maxVideoMem = 0;
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

    hr = D3D11CreateDevice(
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
        );
    
    if(SUCCEEDED(hr))
    {
        com_ptr<ID3D10Multithread> pMultithread;
        hr =  ((*ppDevice)->QueryInterface(IID_PPV_ARGS(pMultithread.put())));

        if(SUCCEEDED(hr))
        {
            pMultithread->SetMultithreadProtected(TRUE);
        }

        
    }

    return hr;
}

HRESULT CreateD3DManager()
{
    HRESULT hr = S_OK;
    D3D_FEATURE_LEVEL FeatureLevel;
    com_ptr<ID3D11DeviceContext> pDX11DeviceContext;
    
    hr = CreateDX11Device(g_pDX11Device.put(), pDX11DeviceContext.put(), &FeatureLevel);

    if(SUCCEEDED(hr))
    {
        hr = MFCreateDXGIDeviceManager(&g_ResetToken, g_pDXGIMan.put());
    }

    if(SUCCEEDED(hr))
    {
        hr = g_pDXGIMan->ResetDevice(g_pDX11Device.get(), g_ResetToken);
    }
        
    return hr;
}

HRESULT
CaptureManager::InitializeCaptureManager(HWND hwndPreview, IUnknown* pUnk)
{
    HRESULT                         hr = S_OK;
    com_ptr<IMFAttributes>          pAttributes;
    com_ptr<IMFCaptureEngineClassFactory>   pFactory;

    DestroyCaptureEngine();


    m_hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (NULL == m_hEvent)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }

    m_pCallback.attach(new (std::nothrow) CaptureEngineCB(m_hwndEvent));
    //m_pCallback = new (std::nothrow) CaptureEngineCB(m_hwndEvent);
    if (m_pCallback == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    m_pCallback->m_pManager = this;
    m_hwndPreview = hwndPreview;

    //Create a D3D Manager
    hr = CreateD3DManager();
    if (FAILED(hr))
    {
        goto Exit;
    }
    hr = MFCreateAttributes(pAttributes.put(), 1);
    if (FAILED(hr))
    {
        goto Exit;
    }
    hr = pAttributes->SetUnknown(MF_CAPTURE_ENGINE_D3D_MANAGER, g_pDXGIMan.get());
    if (FAILED(hr))
    {
        goto Exit;
    }

    // Create the factory object for the capture engine.
    hr = CoCreateInstance(CLSID_MFCaptureEngineClassFactory, NULL, 
        CLSCTX_INPROC_SERVER, IID_PPV_ARGS(pFactory.put()));
    if (FAILED(hr))
    {
        goto Exit;
    }

    // Create and initialize the capture engine.
    hr = pFactory->CreateInstance(CLSID_MFCaptureEngine, IID_PPV_ARGS(m_pEngine.put()));
    if (FAILED(hr))
    {
        goto Exit;
    }
    hr = m_pEngine->Initialize(m_pCallback.get(), pAttributes.get(), NULL, pUnk);
    if (FAILED(hr))
    {
        goto Exit;
    }

Exit:
    return hr;
}

// Handle an event from the capture engine. 
// NOTE: This method is called from the application's UI thread. 
HRESULT CaptureManager::OnCaptureEvent(WPARAM wParam, LPARAM lParam)
{
    GUID guidType;
    HRESULT hrStatus;

    com_ptr<IMFMediaEvent> pEvent;
    pEvent.copy_from(reinterpret_cast<IMFMediaEvent*>(wParam));

    HRESULT hr = pEvent->GetStatus(&hrStatus);
    if (FAILED(hr))
    {
        hrStatus = hr;
    }

    hr = pEvent->GetExtendedType(&guidType);
    if (SUCCEEDED(hr))
    {

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

    HRESULT hr = S_OK;
    
    // Get a pointer to the preview sink.
    if (m_pPreview == NULL)
    {
        hr = m_pEngine->GetSink(MF_CAPTURE_ENGINE_SINK_TYPE_PREVIEW, pSink.put());
        if (FAILED(hr))
        {
            goto done;
        }

        hr = pSink->QueryInterface(IID_PPV_ARGS(m_pPreview.put()));
        if (FAILED(hr))
        {
            goto done;
        }

        hr = m_pPreview->SetRenderHandle(m_hwndPreview);
        if (FAILED(hr))
        {
            goto done;
        }

        hr = m_pEngine->GetSource(pSource.put());
        if (FAILED(hr))
        {
            goto done;
        }

        // Configure the video format for the preview sink.
        hr = pSource->GetCurrentDeviceMediaType((DWORD)MF_CAPTURE_ENGINE_PREFERRED_SOURCE_STREAM_FOR_VIDEO_PREVIEW , pMediaType.put());
        if (FAILED(hr))
        {
            goto done;
        }

        // Add the transform 
        com_ptr<IMFTransform>pMFT;
        hr = TransformAsync::CreateInstance(pMFT.put());

        // IMFCaptureSource
        hr = pSource->AddEffect(0, pMFT.get());
        if (FAILED(hr))
        {
            goto done;
        }

        hr = CloneVideoMediaType(pMediaType.get(), MFVideoFormat_RGB32, pMediaType2.put());
        if (FAILED(hr))
        {
            goto done;
        }

        hr = pMediaType2->SetUINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, TRUE);
        if (FAILED(hr))
        {
            goto done;
        }

        // Connect the video stream to the preview sink.
        DWORD dwSinkStreamIndex;
        hr = m_pPreview->AddStream((DWORD)MF_CAPTURE_ENGINE_PREFERRED_SOURCE_STREAM_FOR_VIDEO_PREVIEW,  pMediaType2.get(), NULL, &dwSinkStreamIndex);        
        if (FAILED(hr))
        {
            goto done;
        }
    }


    hr = m_pEngine->StartPreview();
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
done:

    return hr;
}

HRESULT CaptureManager::StopPreview()
{
    HRESULT hr = S_OK;

    if (m_pEngine == NULL)
    {
        return MF_E_NOT_INITIALIZED;
    }

    if (!m_bPreviewing)
    {
        return S_OK;
    }
    hr = m_pEngine->StopPreview();
    if (FAILED(hr))
    {
        goto done;
    }
    WaitForResult();

    if (m_fPowerRequestSet && m_hpwrRequest != INVALID_HANDLE_VALUE)
    {
        PowerClearRequest(m_hpwrRequest, PowerRequestExecutionRequired);
        m_fPowerRequestSet = false;
    }
done:
    return hr;
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


HRESULT GetEncodingBitrate(IMFMediaType *pMediaType, UINT32 *uiEncodingBitrate)
{
    UINT32 uiWidth;
    UINT32 uiHeight;
    float uiBitrate;
    UINT32 uiFrameRateNum;
    UINT32 uiFrameRateDenom;

    HRESULT hr = GetFrameSize(pMediaType, &uiWidth, &uiHeight);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = GetFrameRate(pMediaType, &uiFrameRateNum, &uiFrameRateDenom);
    if (FAILED(hr))
    {
        goto done;
    }

    uiBitrate = uiWidth / 3.0f * uiHeight * uiFrameRateNum / uiFrameRateDenom;
    
    *uiEncodingBitrate = (UINT32) uiBitrate;

done:

    return hr;
}



