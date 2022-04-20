#include "TransformAsync.h"
#include <winrt/windows.media.h>
#include <winrt/windows.foundation.h>
#include <windows.graphics.directx.direct3d11.interop.h>
#include <winrt/Windows.Graphics.DirectX.Direct3D11.h>
#include <windows.graphics.imaging.interop.h>
#include <windows.media.core.interop.h>
#include "Helpers/common.h"
#include <unknwn.h>
#include <stdio.h>
#include <wchar.h>
#include <future>

#define MFT_NUM_DEFAULT_ATTRIBUTES  4
using namespace MainWindow;

long long g_now; // The time since the last call to FrameThreadProc

// Video FOURCC codes.
const FOURCC FOURCC_NV12 = MAKEFOURCC('N', 'V', '1', '2');
const FOURCC FOURCC_RGB24 = 20;
const FOURCC FOURCC_RGB32 = 22;
float runningAverage = 0.0f;

// static array of media types (preferred and accepted).
const GUID* g_MediaSubtypes[] =
{
    //&MFVideoFormat_NV12,
    &MFVideoFormat_RGB32
};

// number of media types in the aray.
DWORD g_cNumSubtypes = ARRAY_SIZE(g_MediaSubtypes);
HRESULT GetImageSize(FOURCC fcc, UINT32 width, UINT32 height, DWORD* pcbImage);

HRESULT TransformAsync::CreateInstance(IMFTransform** ppMFT) noexcept try
{
    com_ptr<TransformAsync> pMFT;
    if (ppMFT == NULL)
    {
        return E_POINTER;
    }
    
    pMFT.attach(new TransformAsync());

    THROW_IF_FAILED(pMFT->InitializeTransform());
    THROW_IF_FAILED(pMFT->QueryInterface(IID_IMFTransform, (void**)ppMFT));

   return S_OK;
}CATCH_RETURN();

TransformAsync::TransformAsync() 
{
    m_nRefCount = 1;
    m_ulSampleCounter = 0;
    m_spInputType = NULL;
    m_spOutputType = NULL;
    m_spAttributes = NULL;
    m_spEventQueue = NULL;
    m_dwStatus = 0;
    m_dwNeedInputCount = 0;
    m_dwHaveOutputCount = 0;
    m_bShutdown = FALSE;
    m_bFirstSample = TRUE;
    m_pInputSampleQueue = NULL;
    m_pOutputSampleQueue = NULL;
    m_videoFOURCC = 0;
    m_imageWidthInPixels = 0; 
    m_imageHeightInPixels = 0;
    m_spDeviceManager = NULL;
}

TransformAsync::~TransformAsync()
{
    assert(m_nRefCount == 0);
    Shutdown();

    m_spInputType.detach();
    m_spOutputType.detach();
   
    CloseHandle(m_hFenceEvent.get());

    if (m_spEventQueue)
    {
        m_spEventQueue->Shutdown();
    }

    if (m_spDeviceManager)
    {
        m_spDeviceManager->CloseDeviceHandle(m_hDeviceHandle.get());
        m_spDeviceManager->Release();
    }
}

#pragma region IUnknown
ULONG TransformAsync::AddRef(void)
{
    return InterlockedIncrement(&m_nRefCount);
}
HRESULT TransformAsync::QueryInterface(REFIID iid, void** ppv)
{
    if (!ppv)
    {
        return E_POINTER;
    }

    else if (iid == __uuidof(IMFTransform))
    {
        *ppv = static_cast<IMFTransform*>(this);
    }
    else if (iid == IID_IMFAttributes)
    {
        *ppv = reinterpret_cast<IMFAttributes*>(this);
    }
    else if (iid == IID_IMFShutdown)
    {
        *ppv = static_cast<IMFShutdown*>(this);
    }
    else if (iid == IID_IMFMediaEventGenerator)
    {
        *ppv = (IMFMediaEventGenerator*)this;
    }
    else if (iid == IID_IMFAsyncCallback)
    {
        *ppv = static_cast<IMFAsyncCallback*>(this);
    }
    else if (iid == IID_IMFVideoSampleAllocatorNotify)
    {
        *ppv = static_cast<IMFVideoSampleAllocatorNotify*>(this);
    }
    else if (iid == __uuidof(TransformAsync))
    {
        *ppv = (TransformAsync*)this;
    }
    else if (iid == IID_IUnknown)
    {
        *ppv = static_cast<IUnknown*>(this);
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
    AddRef();
    return S_OK;
}
ULONG TransformAsync::Release()
{
    ULONG uCount = InterlockedDecrement(&m_nRefCount);
    if (uCount == 0)
    {
        delete this;
    }
    // For thread safety, return a temporary variable.
    return uCount;
}
#pragma endregion IUnknown


HRESULT TransformAsync::ScheduleFrameInference(void)
{
    com_ptr<IMFSample> pInputSample; 
    com_ptr<IMFAsyncCallback> pInferenceTask;
    com_ptr<IMFAsyncResult> pResult;

    // No need to lock, sample queues are thread safe
     /***************************************
        ** Since this in an internal function
        ** we know m_pInputSampleQueue can never be
        ** NULL due to InitializeTransform()
        ***************************************/

    RETURN_IF_FAILED(m_pInputSampleQueue->GetNextSample(pInputSample.put()));
    
    RETURN_IF_FAILED(MFCreateAsyncResult(pInputSample.get(),   // Pointer to object stored in async result
        this,                               // Pointer to IMFAsyncCallback interface
        this,                               // Pointer to IUnkonwn of state object
        pResult.put()));


    // Schedule the inference task with the MF async work queue. Will call TransformAsync::Invoke when run.
    RETURN_IF_FAILED(MFPutWorkItemEx(MFASYNC_CALLBACK_QUEUE_MULTITHREADED, pResult.get()));
    return S_OK;
}

// SubmitEval returns an HRESULT to use to signal the async result's status
HRESULT TransformAsync::SubmitEval(IMFSample* pInput)
{
    com_ptr<IMFSample> pOutputSample;
    com_ptr<IMFSample> pInputSample;
    IDirect3DSurface src, dest;
    pInputSample.copy_from(pInput); 

    // Select the next available model
    DWORD dwCurrentSample = InterlockedIncrement(&m_ulSampleCounter);
    modelIndex = (++modelIndex) % m_numThreads;
    auto model = m_models[modelIndex].get();

    //pInputSample attributes to copy over to pOutputSample
    LONGLONG hnsDuration = 0;
    LONGLONG hnsTime = 0;
    UINT64 pun64MarkerID = 0;

    TRACE((L"\n[Sample: %d | model: %d | ", dwCurrentSample, modelIndex));
    TRACE((L" | SE Thread %d | ", std::hash<std::thread::id>()(std::this_thread::get_id())));

     // Ensure we still have a valid d3d device
    RETURN_IF_FAILED(CheckDX11Device());
    // Ensure the allocator is set up 
    RETURN_IF_FAILED(SetupAlloc());

    // CheckDX11Device & SetupAlloc ensure we can now allocate a D3D-backed output sample. 
    HRESULT hr = m_spOutputSampleAllocator->AllocateSample(pOutputSample.put());
    if (FAILED(hr))
    {
        TRACE((L"Can't allocate any more samples, DROP"));
    }

    // We should have a sample now, whether or not we have a dx device
    RETURN_IF_NULL_ALLOC(pOutputSample);

    // Explicitly copy out pInput attributes, since copying IMFSamples is shallow. 
    RETURN_IF_FAILED(pInputSample->GetSampleDuration(&hnsDuration));
    RETURN_IF_FAILED(pInputSample->GetSampleTime(&hnsTime));
    pInputSample->GetUINT64(TransformAsync_MFSampleExtension_Marker, &pun64MarkerID);

    // Extract an IDirect3DSurface from the input and output samples to use for inference, 
    src = SampleToD3Dsurface(pInputSample.get());
    dest = SampleToD3Dsurface(pOutputSample.get());

    // Make sure this model isn't already running, if so drop the frame and move on. 
    if(model->m_bSyncStarted == FALSE)
    {
        auto now = std::chrono::high_resolution_clock::now();
        // Run model inference 
        model->Run(src, dest); 
        auto timePassed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - now);
        TRACE((L"Eval: %d", timePassed.count()));
        src.Close();
        dest.Close();
        // Perform housekeeping out the output sample
        RETURN_IF_FAILED(FinishEval(pInputSample, pOutputSample, hnsDuration, hnsTime, pun64MarkerID)); 
    }
    else {
        TRACE((L"Model %d already running, DROP", modelIndex));
    }

    return S_OK;
}

// Clean up output sample and schedule MFHaveOutput task
HRESULT TransformAsync::FinishEval(com_ptr<IMFSample> pInputSample, com_ptr<IMFSample> pOutputSample,
    LONGLONG hnsDuration, LONGLONG hnsTime, UINT64 pun64MarkerID)
{
    com_ptr<IMFMediaBuffer> pMediaBuffer;
    com_ptr<IMFMediaEvent> pHaveOutputEvent;

    // Set up the output sample
    RETURN_IF_FAILED(pOutputSample->SetSampleDuration(hnsDuration));
    RETURN_IF_FAILED(pOutputSample->SetSampleTime(hnsTime));

    // Always set the output buffer size!
    RETURN_IF_FAILED(pOutputSample->GetBufferByIndex(0, pMediaBuffer.put()));
    RETURN_IF_FAILED(pMediaBuffer->SetCurrentLength(m_cbImageSize));

    // This is the first sample after a gap in the stream. 
    if(m_bFirstSample != FALSE)
    {
        RETURN_IF_FAILED(pOutputSample->SetUINT32(MFSampleExtension_Discontinuity, TRUE));
        m_bFirstSample = FALSE;
    }

    // Allocate output sample to queue
    RETURN_IF_FAILED(m_pOutputSampleQueue->AddSample(pOutputSample.get()));
    RETURN_IF_FAILED(MFCreateMediaEvent(METransformHaveOutput, GUID_NULL, S_OK, NULL, pHaveOutputEvent.put()));
    // Scope event queue lock
    {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        // TODO: If QueueEvent fails, consider decrementing m_dwHaveOutputCount
        RETURN_IF_FAILED(m_spEventQueue->QueueEvent(pHaveOutputEvent.get()));
        m_dwHaveOutputCount++;
        m_dwStatus |= MYMFT_STATUS_OUTPUT_SAMPLE_READY;
    }

    if (pun64MarkerID)
    {
        // This input sample is flagged as a marker
        com_ptr<IMFMediaEvent> pMarkerEvent;
        RETURN_IF_FAILED(MFCreateMediaEvent(METransformMarker, GUID_NULL, S_OK, NULL, pMarkerEvent.put()));
        RETURN_IF_FAILED(pMarkerEvent->SetUINT64(MF_EVENT_MFT_CONTEXT, pun64MarkerID));
        RETURN_IF_FAILED(m_spEventQueue->QueueEvent(pMarkerEvent.get()));
    }

    // Done processing this sample, request another
    RETURN_IF_FAILED(RequestSample(0));
    InterlockedIncrement(&m_ulProcessedFrameNum); 
    return S_OK;
}


DWORD __stdcall FrameThreadProc(LPVOID lpParam) 
{
    DWORD dwWaitResult; 
    // Get the handle from the lpParam pointer
    HANDLE hEvent = lpParam;   
    //OutputDebugString(L"Thread %d waiting for Frame event...");
    dwWaitResult = WaitForSingleObject(
        hEvent,         // event handle
        INFINITE);      // indefinite wait
   

    switch (dwWaitResult) {
    case WAIT_OBJECT_0:
        // TODO: Capture time and write to preview
        if (g_now == NULL){
            g_now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
            //OutputDebugString(L"First time responding to event!");
        }
        else {
            auto l_now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
            auto timePassed = l_now - g_now;
            g_now = l_now;
            auto fps = 30000 / timePassed; // TODO: marco on num frames to update after? 
            OutputDebugString(L"THREAD: ");
            OutputDebugString(std::to_wstring(fps).c_str());
            OutputDebugString(L"\n");

            auto message = std::wstring(L"Frame Rate: ") + std::to_wstring(fps) + L" FPS";
            MainWindow::_SetStatusText(message.c_str());
            //TRACE(("Responded to event and it's been %d miliseconds", timePassed));
            // TODO: Call Set status text with new framerate
        }

        break;
    default: 
        TRACE(("Wait error (%d)\n", GetLastError()));
        return 0;
    }
    return 1;
}

HRESULT TransformAsync::OnSetD3DManager(ULONG_PTR ulParam)
{
    com_ptr<IUnknown> pUnk;
    com_ptr<IMFDXGIDeviceManager> pDXGIDeviceManager;
    com_ptr<ID3D11DeviceContext> pContext;
    com_ptr<ID3D11Device5> pDevice;
    if (ulParam != NULL) {
        pUnk.attach((IUnknown*)ulParam);
        pDXGIDeviceManager = pUnk.try_as<IMFDXGIDeviceManager>();
        if (pDXGIDeviceManager && m_spDeviceManager != pDXGIDeviceManager) {
            // Invalidate dx11 resources
            InvalidateDX11Resources();

            // Update dx11 device and resources
            m_spDeviceManager = pDXGIDeviceManager;
            RETURN_IF_FAILED(UpdateDX11Device());

            // Hand off the new device to the sample allocator, if it exists
            if (m_spOutputSampleAllocator)
            {
                RETURN_IF_FAILED(m_spOutputSampleAllocator->SetDirectXManager(pUnk.get()));
            }

            // Create event and thread for framerate
            m_hFenceEvent.reset(CreateEvent(NULL,               // Security attributes
                FALSE,              // Reset token to false means system will auto-reset event object
                FALSE,              // Initial state is nonsignaled
                TEXT("FrameEvent")));  // Event object name
            
            DWORD dwThreadID;
        }
    }
    else
    {
        InvalidateDX11Resources();
    }

    return S_OK;
}

HRESULT TransformAsync::UpdateDX11Device()
{
    com_ptr<ID3D11DeviceContext> pContext;
    com_ptr<ID3D11Device> pDevice;

    if (m_spDeviceManager != nullptr )
    {
        RETURN_IF_FAILED(m_spDeviceManager->OpenDeviceHandle(m_hDeviceHandle.addressof()));
        RETURN_IF_FAILED(m_spDeviceManager->GetVideoService(m_hDeviceHandle.get(), IID_PPV_ARGS(&pDevice)));
        m_spDevice = pDevice.try_as<ID3D11Device5>();
        m_spDevice->GetImmediateContext(pContext.put());
        m_spContext = pContext.try_as<ID3D11DeviceContext4>();

        // Create a fence to use for tracking framerate
        m_fenceValue = 0;
        D3D11_FENCE_FLAG flag = D3D11_FENCE_FLAG_NONE;
        m_spDevice->CreateFence(m_fenceValue, flag, __uuidof(ID3D11Fence), m_spFence.put_void());
    }
    else {
        InvalidateDX11Resources();
    }

    return S_OK;

}

void TransformAsync::InvalidateDX11Resources()
{
    m_spDevice = nullptr;
    m_spContext = nullptr;
}

HRESULT TransformAsync::SetupAlloc()
{
    // Fail fast if already set up 
    if (!m_bAllocatorInitialized)
    {
        // If we have a device manager, we need to set up sample allocator
        if (m_spDeviceManager != nullptr)
        {
            // Set up allocator attributes
            DWORD dwBindFlags = MFGetAttributeUINT32(m_spAllocatorAttributes.get(), MF_SA_D3D11_BINDFLAGS, D3D11_BIND_RENDER_TARGET);
            dwBindFlags |= D3D11_BIND_RENDER_TARGET; // Must always set as render target!
            RETURN_IF_FAILED(m_spAllocatorAttributes->SetUINT32(MF_SA_D3D11_BINDFLAGS, dwBindFlags));
            RETURN_IF_FAILED(m_spAllocatorAttributes->SetUINT32(MF_SA_BUFFERS_PER_SAMPLE, 1));
            RETURN_IF_FAILED(m_spAllocatorAttributes->SetUINT32(MF_SA_D3D11_USAGE, D3D11_USAGE_DEFAULT));

            // Set up the output sample allocator if needed
            if (NULL == m_spOutputSampleAllocator)
            {
                com_ptr<IMFVideoSampleAllocatorEx> spVideoSampleAllocator;
                com_ptr<IUnknown> spDXGIManagerUnk;

                RETURN_IF_FAILED(MFCreateVideoSampleAllocatorEx(IID_PPV_ARGS(&spVideoSampleAllocator)));
                spDXGIManagerUnk = m_spDeviceManager.as<IUnknown>();

                RETURN_IF_FAILED(spVideoSampleAllocator->SetDirectXManager(spDXGIManagerUnk.get()));

                m_spOutputSampleAllocator.attach(spVideoSampleAllocator.detach());
            }

            // TODO: This should prob raise an error if it doesn't work
            RETURN_IF_FAILED(m_spOutputSampleAllocator->InitializeSampleAllocatorEx(2, m_numThreads, m_spAllocatorAttributes.get(), m_spOutputType.get()));

            // Set up IMFVideoSampleAllocatorCallback
            m_spOutputSampleCallback = m_spOutputSampleAllocator.try_as<IMFVideoSampleAllocatorCallback>();
            m_spOutputSampleCallback->SetCallback(this); // TODO: Will setCallback QI for Notify ? 
        }
        m_bAllocatorInitialized = true;

    }

    return S_OK;
}

HRESULT TransformAsync::CheckDX11Device()
{
    HRESULT hr = S_OK;

    if (m_spDeviceManager != nullptr && m_hDeviceHandle)
    {
        if (m_spDeviceManager->TestDevice(m_hDeviceHandle.get()) != S_OK)
        {
            InvalidateDX11Resources();
            UpdateDX11Device();
        }
    }
    return S_OK;
}


HRESULT TransformAsync::OnGetPartialType(DWORD dwTypeIndex, IMFMediaType** ppmt)
{
    if (dwTypeIndex >= g_cNumSubtypes)
    {
        return MF_E_NO_MORE_TYPES;
    }

    com_ptr<IMFMediaType> pmt;

    RETURN_IF_FAILED(MFCreateMediaType(pmt.put()));

    RETURN_IF_FAILED(pmt->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video));

    RETURN_IF_FAILED(pmt->SetGUID(MF_MT_SUBTYPE, *g_MediaSubtypes[dwTypeIndex]));

    *ppmt = pmt.get();
    (*ppmt)->AddRef();

    return S_OK;
}

HRESULT TransformAsync::OnCheckInputType(IMFMediaType* pmt)
{
    TRACE((L"OnCheckInputType\n"));
    assert(pmt != NULL);

    HRESULT hr = S_OK;

    // If the output type is set, see if they match.
    if (m_spOutputType != NULL)
    {
        DWORD flags = 0;
        hr = pmt->IsEqual(m_spOutputType.get(), &flags);

        // IsEqual can return S_FALSE. Treat this as failure.

        if (hr != S_OK)
        {
            hr = MF_E_INVALIDMEDIATYPE;
        }
    }
    else
    {
        // Output type is not set. Just check this type.
        hr = OnCheckMediaType(pmt);
    }

    return hr;
}


//-------------------------------------------------------------------
// Name: OnCheckOutputType
// Description: Validate an output media type.
//-------------------------------------------------------------------

HRESULT TransformAsync::OnCheckOutputType(IMFMediaType* pmt)
{
    TRACE((L"OnCheckOutputType\n"));
    assert(pmt != NULL);

    HRESULT hr = S_OK;

    // If the input type is set, see if they match.
    if (m_spInputType != NULL)
    {
        DWORD flags = 0;
        hr = pmt->IsEqual(m_spInputType.get(), &flags);

        // IsEqual can return S_FALSE. Treat this as failure.

        if (hr != S_OK)
        {
            hr = MF_E_INVALIDMEDIATYPE;
        }

    }
    else
    {
        // Input type is not set. Just check this type.
        hr = OnCheckMediaType(pmt);
    }

    return hr;
}

//-------------------------------------------------------------------
// Name: OnCheckMediaType
// Description: Validates a media type for this transform.
//-------------------------------------------------------------------
HRESULT TransformAsync::OnCheckMediaType(IMFMediaType* pmt)
{
    LogMediaType(pmt);

    GUID major_type = GUID_NULL;
    GUID subtype = GUID_NULL;
    MFVideoInterlaceMode interlace = MFVideoInterlace_Unknown;
    UINT32 val = 0;
    BOOL bFoundMatchingSubtype = FALSE;

    // Major type must be video.
    RETURN_IF_FAILED(pmt->GetGUID(MF_MT_MAJOR_TYPE, &major_type));

    if (major_type != MFMediaType_Video)
    {
        RETURN_IF_FAILED(MF_E_INVALIDMEDIATYPE);
    }

    // Subtype must be one of the subtypes in our global list.

    // Get the subtype GUID.
    RETURN_IF_FAILED(pmt->GetGUID(MF_MT_SUBTYPE, &subtype));

    // Look for the subtype in our list of accepted types.
    for (DWORD i = 0; i < g_cNumSubtypes; i++)
    {
        if (subtype == *g_MediaSubtypes[i])
        {
            bFoundMatchingSubtype = TRUE;
            break;
        }
    }

    if (!bFoundMatchingSubtype)
    {
        RETURN_IF_FAILED(MF_E_INVALIDMEDIATYPE);
    }

    // Video must be progressive frames.
    RETURN_IF_FAILED(pmt->GetUINT32(MF_MT_INTERLACE_MODE, (UINT32*)&interlace));
    if (!(interlace == MFVideoInterlace_Progressive /* || interlace == MFVideoInterlace_MixedInterlaceOrProgressive*/))
    {
        RETURN_IF_FAILED(MF_E_INVALIDMEDIATYPE);
    }

    return S_OK;
}

//-------------------------------------------------------------------
// Name: OnSetInputType
// Description: Sets or clears the input media type.
//
// Prerequisite:
// The input type has already been validated.
//-------------------------------------------------------------------

HRESULT TransformAsync::OnSetInputType(IMFMediaType* pmt)
{
    TRACE((L"TransformAsync::OnSetInputType\n"));

    m_spInputType.detach();
    m_spInputType.attach(pmt);

    // Update the format information.
    UpdateFormatInfo();

    return S_OK;
}


//-------------------------------------------------------------------
// Name: OnSetOutputType
// Description: Sets or clears the output media type.
//
// Prerequisite:
// The output type has already been validated.
//-------------------------------------------------------------------

HRESULT TransformAsync::OnSetOutputType(IMFMediaType* pmt)
{
    TRACE((L"TransformAsync::OnSetOutputType\n"));

    // if pmt is NULL, clear the type.
    // if pmt is non-NULL, set the type.

    m_spOutputType.detach();
    m_spOutputType.attach(pmt);

    return S_OK;
}


//-------------------------------------------------------------------
// Name: SampleToD3DSurface
// Description: Convert an IMFSample to an IDirect3DSurface
// 
// IMFSample -> IMFMediaBuffer          Get underlying buffer
// IMFMediaBuffer -> IMFDXGIBuffer      QI to get the DXGI-backed buffer
// IMFDXGIBuffer -> ID3D11Texture2D     Get texture resources from DXGI-backed buffer
// ID3D11Texture2D -> IDXGISurface      QI to get DXGI surface from texture resource
// IDXGISurface -> IDirect3DSurface     DXGI-D3D interop so can create a VideoFrame for WinML
//
//-------------------------------------------------------------------
IDirect3DSurface TransformAsync::SampleToD3Dsurface(IMFSample* sample)
{
    com_ptr<IMFMediaBuffer> pBuffIn;
    com_ptr<IMFDXGIBuffer> pSrc;
    com_ptr<ID3D11Texture2D> pTextSrc;
    com_ptr<IDXGISurface> pSurfaceSrc;

    com_ptr<IInspectable> pSurfaceInspectable;

    sample->ConvertToContiguousBuffer(pBuffIn.put());
    pBuffIn->QueryInterface(IID_PPV_ARGS(&pSrc));
    pSrc->GetResource(IID_PPV_ARGS(&pTextSrc));
    pTextSrc->QueryInterface(IID_PPV_ARGS(&pSurfaceSrc));

    CreateDirect3D11SurfaceFromDXGISurface(pSurfaceSrc.get(), pSurfaceInspectable.put());
    return pSurfaceInspectable.try_as<IDirect3DSurface>();
}

HRESULT TransformAsync::InitializeTransform(void)
{

    RETURN_IF_FAILED(MFCreateAttributes(m_spAttributes.put(), MFT_NUM_DEFAULT_ATTRIBUTES));
    RETURN_IF_FAILED(m_spAttributes->SetUINT32(MF_TRANSFORM_ASYNC, TRUE));
    RETURN_IF_FAILED(m_spAttributes->SetUINT32(MFT_SUPPORT_DYNAMIC_FORMAT_CHANGE, TRUE));
    RETURN_IF_FAILED(m_spAttributes->SetUINT32(MF_SA_D3D_AWARE, TRUE));
    RETURN_IF_FAILED(m_spAttributes->SetUINT32(MF_SA_D3D11_AWARE, TRUE));

    // Initialize attributes for output sample allocator
    RETURN_IF_FAILED(MFCreateAttributes(m_spAllocatorAttributes.put(), 3));

    /**********************************
    ** Since this is an Async MFT, an
    ** event queue is required
    ** MF Provides a standard implementation
    **********************************/
    RETURN_IF_FAILED(MFCreateEventQueue(m_spEventQueue.put()));

    RETURN_IF_FAILED(CSampleQueue::Create(&m_pInputSampleQueue));

    RETURN_IF_FAILED(CSampleQueue::Create(&m_pOutputSampleQueue));

    // Set up circular queue of StreamModelBases
    for (int i = 0; i < m_numThreads; i++) {
        // TODO: Have a dialogue to select which model to select for real-time inference. 
        m_models.push_back(std::make_unique<BackgroundBlur>());
    }

    return S_OK;
}

HRESULT TransformAsync::ShutdownEventQueue(void)
{
    /***************************************
        ** Since this in an internal function
        ** we know m_spEventQueue can never be
        ** NULL due to InitializeTransform()
        ***************************************/

    RETURN_IF_FAILED(m_spEventQueue->Shutdown());
    return S_OK;
}

//-------------------------------------------------------------------
// Name: GetImageSize
// Description: 
// Calculates the buffer size needed, based on the video format.
//-------------------------------------------------------------------
HRESULT GetImageSize(FOURCC fcc, UINT32 width, UINT32 height, DWORD* pcbImage)
{
    HRESULT hr = S_OK;

    switch (fcc)
    {
    case FOURCC_NV12:
        // check overflow
        if ((height / 2 > MAXDWORD - height) ||
            ((height + height / 2) > MAXDWORD / width))
        {
            hr = E_INVALIDARG;
        }
        else
        {
            // 12 bpp
            *pcbImage = width * (height + (height / 2));
        }
        break;

    case FOURCC_RGB24:
        // check overflow
        if ((width * 3 > MAXDWORD) ||
            (width * 3 > MAXDWORD / height))
        {
            hr = E_INVALIDARG;
        }
        else {
            // 24 bpp
            *pcbImage = width * height * 3;
        }
        break;

    case FOURCC_RGB32:
        // check overflow
        if ((width * 4 > MAXDWORD) ||
            (width * 4 > MAXDWORD / height))
        {
            hr = E_INVALIDARG;
        }
        else {
            // 32 bpp
            *pcbImage = width * height * 4;
        }
        break;
    default:
        hr = E_FAIL;    // Unsupported type.
    }

    return hr;
}

HRESULT TransformAsync::UpdateFormatInfo()
{

    GUID subtype = GUID_NULL;

    m_imageWidthInPixels = 0;
    m_imageHeightInPixels = 0;
    m_videoFOURCC = 0;
    m_cbImageSize = 0;

    if (m_spInputType != NULL)
    {
        RETURN_IF_FAILED(m_spInputType->GetGUID(MF_MT_SUBTYPE, &subtype));

        m_videoFOURCC = subtype.Data1;

        RETURN_IF_FAILED(MFGetAttributeSize(
            m_spInputType.get(),
            MF_MT_FRAME_SIZE,
            &m_imageWidthInPixels,
            &m_imageHeightInPixels
        ));

        TRACE((L"Frame size: %d x %d\n", m_imageWidthInPixels, m_imageHeightInPixels));

        // Calculate the image size (not including padding)
        RETURN_IF_FAILED(GetImageSize(m_videoFOURCC, m_imageWidthInPixels, m_imageHeightInPixels, &m_cbImageSize));

        // Set the size of the SegmentModel
        for (int i = 0; i < m_numThreads; i++)
        {
            m_models[i]->InitializeSession(m_imageWidthInPixels, m_imageHeightInPixels);
        }
    }

    return S_OK;
}

HRESULT TransformAsync::OnStartOfStream(void)
{
    {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);

        m_dwStatus |= MYMFT_STATUS_STREAM_STARTED;

    }

    /*******************************
    ** Note: This MFT only has one
    ** input stream, so RequestSample
    ** is always called with '0'. If
    ** your MFT has more than one
    ** input stream, you will
    ** have to change this logic
    *******************************/
    RETURN_IF_FAILED(RequestSample(0));

    return S_OK;
}

HRESULT TransformAsync::OnEndOfStream(void)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    m_dwStatus &= (~MYMFT_STATUS_STREAM_STARTED);

    /*****************************************
    ** See http://msdn.microsoft.com/en-us/library/dd317909(VS.85).aspx#processinput
    ** Upon receiving EOS, the outstanding process
    ** input request should be reset to 0
    *****************************************/
    m_dwNeedInputCount = 0;
    return S_OK;
}

HRESULT TransformAsync::OnDrain(
    const UINT32 un32StreamID)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    if (m_pOutputSampleQueue->IsQueueEmpty())
    {
        com_ptr<IMFMediaEvent> pDrainCompleteEvent;
        RETURN_IF_FAILED(MFCreateMediaEvent(METransformDrainComplete, GUID_NULL, S_OK, NULL, pDrainCompleteEvent.put()));

        /*******************************
        ** Note: This MFT only has one
        ** input stream, so the drain
        ** is always on stream zero.
        ** Update this is your MFT
        ** has more than one stream
        *******************************/
        RETURN_IF_FAILED(pDrainCompleteEvent->SetUINT32(MF_EVENT_MFT_INPUT_STREAM_ID, 0));

        /***************************************
        ** Since this in an internal function
        ** we know m_spEventQueue can never be
        ** NULL due to InitializeTransform()
        ***************************************/
        RETURN_IF_FAILED(m_spEventQueue->QueueEvent(pDrainCompleteEvent.get()));
    }
    else
    {
        m_dwStatus |= (MYMFT_STATUS_DRAINING);

    }
    /*******************************
    ** Note: This MFT only has one
    ** input stream, so it does not
    ** track the stream being drained.
    ** If your MFT has more than one
    ** input stream, you will
    ** have to change this logic
    *******************************/

    return S_OK;
}

HRESULT TransformAsync::OnFlush(void)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    m_dwStatus &= (~MYMFT_STATUS_STREAM_STARTED);

    RETURN_IF_FAILED(FlushSamples());

    return S_OK;
}

HRESULT TransformAsync::OnMarker(
    const ULONG_PTR pulID)
{
    // No need to lock, our sample queue is thread safe

       /***************************************
       ** Since this in an internal function
       ** we know m_pInputSampleQueue can never be
       ** NULL due to InitializeTransform()
       ***************************************/

    RETURN_IF_FAILED(m_pInputSampleQueue->MarkerNextSample(pulID));

    return S_OK;
}

HRESULT TransformAsync::RequestSample(
    const UINT32 un32StreamID)
{
    // Fail fast if streaming not started
    {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);

        if ((m_dwStatus & MYMFT_STATUS_STREAM_STARTED) == 0)
        {
            // Stream hasn't started
            return MF_E_NOTACCEPTING;
        }
    }

    com_ptr<IMFMediaEvent> pEvent;
    RETURN_IF_FAILED(MFCreateMediaEvent(METransformNeedInput, GUID_NULL, S_OK, NULL, pEvent.put()));
    
    if (pEvent == NULL)
    {
        TRACE((L"RequestSample: pEvent null not queueing anything!"));
    }

    RETURN_IF_FAILED(pEvent->SetUINT32(MF_EVENT_MFT_INPUT_STREAM_ID, un32StreamID));
    

    /***************************************
    ** Since this in an internal function
    ** we know m_spEventQueue can never be
    ** NULL due to InitializeTransform()
    ***************************************/

    {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);

        RETURN_IF_FAILED(m_spEventQueue->QueueEvent(pEvent.get()));
        m_dwNeedInputCount++;

    }

    return S_OK;
}

BOOL TransformAsync::IsMFTReady(void)
{

    BOOL bReady = FALSE;

    do
    {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);

        m_dwStatus &= (~MYMFT_STATUS_INPUT_ACCEPT_DATA);

        if (m_spInputType == NULL)
        {
            // The Input type is not set
            break;
        }

        if (m_spOutputType == NULL)
        {
            // The output type is not set
            break;
        }

        m_dwStatus |= MYMFT_STATUS_INPUT_ACCEPT_DATA; // The MFT is ready for data

        bReady = TRUE;
    } while (false);

    return bReady;
}

HRESULT TransformAsync::FlushSamples(void)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    RETURN_IF_FAILED(OnEndOfStream());       // Treat this like an end of stream, don't accept new samples unti)l
    m_dwHaveOutputCount = 0;    // Don't Output samples until new input samples are given
    RETURN_IF_FAILED(m_pInputSampleQueue->RemoveAllSamples());
    RETURN_IF_FAILED(m_pOutputSampleQueue->RemoveAllSamples());
    m_bFirstSample = TRUE; // Be sure to reset our first sample so we know to set discontinuity

    return S_OK;
}
