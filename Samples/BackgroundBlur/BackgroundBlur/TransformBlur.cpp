#include "TransformBlur.h"
#include <winrt/windows.media.h>
#include <winrt/windows.foundation.h>
#include <windows.graphics.directx.direct3d11.interop.h>
//#include <windows.graphics.directx.direct3d11.h>
#include <winrt/Windows.Graphics.DirectX.Direct3D11.h>

#include <windows.media.core.interop.h>
#include "common.h"
#include <unknwn.h>

#include <windows.graphics.imaging.interop.h>

using namespace winrt::Windows::Media;

// Video FOURCC codes.
const FOURCC FOURCC_YUY2 = MAKEFOURCC('Y', 'U', 'Y', '2');
const FOURCC FOURCC_UYVY = MAKEFOURCC('U', 'Y', 'V', 'Y');
const FOURCC FOURCC_NV12 = MAKEFOURCC('N', 'V', '1', '2');
const FOURCC FOURCC_RGB24 = 20;
const FOURCC FOURCC_RGB32 = 22;

// Static array of media types (preferred and accepted).
const GUID* g_MediaSubtypes[] =
{
    &MFVideoFormat_RGB32,
    //&MFVideoFormat_NV12,
};

// Number of media types in the aray.
DWORD g_cNumSubtypes = ARRAY_SIZE(g_MediaSubtypes);
HRESULT GetImageSize(FOURCC fcc, UINT32 width, UINT32 height, DWORD* pcbImage);

//-------------------------------------------------------------------
// Name: CreateInstance
// Description: Static method to create an instance of the source.
//
// pUnkOuter:   Aggregating object or NULL.
// iid:         IID of the requested interface on the source.
// ppSource:    Receives a ref-counted pointer to the source.
//-------------------------------------------------------------------
HRESULT TransformBlur::CreateInstance(IUnknown* pUnkOuter, REFIID iid, void** ppMFT)
{
    if (ppMFT == NULL)
    {
        return E_POINTER;
    }

    // This object does not support aggregation.
    if (pUnkOuter != NULL)
    {
        return CLASS_E_NOAGGREGATION;
    }

    HRESULT hr = S_OK;
    TransformBlur* pMFT = new TransformBlur(hr);
    if (pMFT == NULL)
    {
        CHECK_HR(hr = E_OUTOFMEMORY);
    }

    CHECK_HR(hr = pMFT->QueryInterface(iid, ppMFT));

done:
    SAFE_RELEASE(pMFT);
    return hr;
}

// Constructor
TransformBlur::TransformBlur(HRESULT& hr) :
    m_nRefCount(1),
    m_spSample(NULL),
    m_spInputType(NULL),
    m_spOutputType(NULL),
    m_videoFOURCC(0),
    m_imageWidthInPixels(0),
    m_imageHeightInPixels(0),
    m_cbImageSize(0),
    m_spDeviceManager(NULL),
    m_hDeviceHandle(NULL),
    m_spDevice(NULL),
    m_spVideoDevice(NULL),
    m_pAttributes(NULL)
{
    hr = MFCreateAttributes(&m_pAttributes, 2);
    hr = m_pAttributes->SetUINT32(MF_SA_D3D_AWARE, TRUE);
    hr = m_pAttributes->SetUINT32(MF_SA_D3D11_AWARE, TRUE);

    MFCreateAttributes(&m_spOutputAttributes, 3);

}

//Destructor
TransformBlur::~TransformBlur()
{
    assert(m_nRefCount == 0);

    m_spDeviceManager->CloseDeviceHandle(m_hDeviceHandle);
    SAFE_RELEASE(m_pAttributes);
}

// IUnknown methods
ULONG TransformBlur::AddRef()
{
    return InterlockedIncrement(&m_nRefCount);
}

ULONG TransformBlur::Release()
{
    ULONG uCount = InterlockedDecrement(&m_nRefCount);
    if (uCount == 0)
    {
        delete this;
    }
    // For thread safety, return a temporary variable.
    return uCount;
}
HRESULT TransformBlur::QueryInterface(REFIID iid, void** ppv)
{
    if (!ppv)
    {
        return E_POINTER;
    }
    if (iid == IID_IUnknown)
    {
        *ppv = static_cast<IUnknown*>(this);
    }
    else if (iid == __uuidof(IMFTransform))
    {
        *ppv = static_cast<IMFTransform*>(this);
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
    AddRef();
    return S_OK;
}

// IMFTransform Methods. 

//-------------------------------------------------------------------
// Name: GetStreamLimits
// Returns the minimum and maximum number of streams.
//-------------------------------------------------------------------
HRESULT TransformBlur::GetStreamLimits(
    DWORD* pdwInputMinimum,
    DWORD* pdwInputMaximum,
    DWORD* pdwOutputMinimum,
    DWORD* pdwOutputMaximum
)
{

    if ((pdwInputMinimum == NULL) ||
        (pdwInputMaximum == NULL) ||
        (pdwOutputMinimum == NULL) ||
        (pdwOutputMaximum == NULL))
    {
        return E_POINTER;
    }


    // This MFT has a fixed number of streams.
    *pdwInputMinimum = 1;
    *pdwInputMaximum = 1;
    *pdwOutputMinimum = 1;
    *pdwOutputMaximum = 1;

    return S_OK;
}

//-------------------------------------------------------------------
// Name: GetStreamCount
// Returns the actual number of streams.
//-------------------------------------------------------------------
HRESULT TransformBlur::GetStreamCount(
    DWORD* pcInputStreams,
    DWORD* pcOutputStreams
)
{
    if ((pcInputStreams == NULL) || (pcOutputStreams == NULL))

    {
        return E_POINTER;
    }

    // This MFT has a fixed number of streams.
    *pcInputStreams = 1;
    *pcOutputStreams = 1;

    return S_OK;
}

//-------------------------------------------------------------------
// Name: GetStreamIDs
// Returns stream IDs for the input and output streams.
//-------------------------------------------------------------------
HRESULT TransformBlur::GetStreamIDs(
    DWORD   dwInputIDArraySize,
    DWORD* pdwInputIDs,
    DWORD   dwOutputIDArraySize,
    DWORD* pdwOutputIDs
)
{
    // Do not need to implement, because this MFT has a fixed number of 
    // streams and the stream IDs match the stream indexes.
    return E_NOTIMPL;
}

//-------------------------------------------------------------------
// Name: GetInputStreamInfo
// Returns information about an input stream. 
//-------------------------------------------------------------------
HRESULT TransformBlur::GetInputStreamInfo(
    DWORD                     dwInputStreamID,
    MFT_INPUT_STREAM_INFO* pStreamInfo
)
{
    TRACE((L"GetInputStreamInfo\n"));

    AutoLock lock(m_critSec);

    if (pStreamInfo == NULL)
    {
        return E_POINTER;
    }

    if (!IsValidInputStream(dwInputStreamID))
    {
        return MF_E_INVALIDSTREAMNUMBER;
    }

    // NOTE: This method should succeed even when there is no media type on the
    //       stream. If there is no media type, we only need to fill in the dwFlags 
    //       member of MFT_INPUT_STREAM_INFO. The other members depend on having a
    //       a valid media type.

    pStreamInfo->hnsMaxLatency = 0;
    pStreamInfo->dwFlags = MFT_INPUT_STREAM_WHOLE_SAMPLES | MFT_INPUT_STREAM_SINGLE_SAMPLE_PER_BUFFER;

    if (m_spInputType == NULL)
    {
        pStreamInfo->cbSize = 0;
    }
    else
    {
        pStreamInfo->cbSize = m_cbImageSize;
    }

    pStreamInfo->cbMaxLookahead = 0;
    pStreamInfo->cbAlignment = 0;

    return S_OK;
}

//-------------------------------------------------------------------
// Name: GetOutputStreamInfo
// Returns information about an output stream. 
//-------------------------------------------------------------------
HRESULT TransformBlur::GetOutputStreamInfo(
    DWORD                     dwOutputStreamID,
    MFT_OUTPUT_STREAM_INFO* pStreamInfo
)
{
    AutoLock lock(m_critSec);

    if (pStreamInfo == NULL)
    {
        return E_POINTER;
    }

    if (!IsValidOutputStream(dwOutputStreamID))
    {
        return MF_E_INVALIDSTREAMNUMBER;
    }

    // NOTE: This method should succeed even when there is no media type on the
    //       stream. If there is no media type, we only need to fill in the dwFlags 
    //       member of MFT_OUTPUT_STREAM_INFO. The other members depend on having a
    //       a valid media type.

    pStreamInfo->dwFlags =
        MFT_OUTPUT_STREAM_WHOLE_SAMPLES |
        MFT_OUTPUT_STREAM_SINGLE_SAMPLE_PER_BUFFER |
        MFT_OUTPUT_STREAM_FIXED_SAMPLE_SIZE  |
        MFT_OUTPUT_STREAM_PROVIDES_SAMPLES
        ;

    if (m_spOutputType == NULL)
    {
        pStreamInfo->cbSize = 0;
    }
    else
    {
        pStreamInfo->cbSize = m_cbImageSize;
    }

    pStreamInfo->cbAlignment = 0;

    return S_OK;
}

//-------------------------------------------------------------------
// Name: GetAttributes
// Returns the attributes for the MFT.
//-------------------------------------------------------------------
HRESULT TransformBlur::GetAttributes(IMFAttributes** ppAttributes)
{
    if (ppAttributes == NULL)
    {
        return E_POINTER;
    }

    AutoLock lock (m_critSec);

    *ppAttributes = m_pAttributes;
    (*ppAttributes)->AddRef();

    return S_OK;
}


//-------------------------------------------------------------------
// Name: GetInputStreamAttributes
// Returns stream-level attributes for an input stream.
//-------------------------------------------------------------------
HRESULT TransformBlur::GetInputStreamAttributes(
    DWORD           dwInputStreamID,
    IMFAttributes** ppAttributes
)
{
    // This MFT does not support any attributes, so the method is not implemented.
    return E_NOTIMPL;
}

//-------------------------------------------------------------------
// Name: GetOutputStreamAttributes
// Returns stream-level attributes for an output stream.
//-------------------------------------------------------------------
HRESULT TransformBlur::GetOutputStreamAttributes(
    DWORD           dwOutputStreamID,
    IMFAttributes** ppAttributes
)
{
    return E_NOTIMPL;
    // TODO: Does this screw with output ? 
    HRESULT hr = MFCreateAttributes(ppAttributes, 2);
    hr = (*ppAttributes)->SetUINT32(MF_SA_MINIMUM_OUTPUT_SAMPLE_COUNT, 1);
    hr = (*ppAttributes)->SetUINT32(MF_SA_MINIMUM_OUTPUT_SAMPLE_COUNT_PROGRESSIVE, 1);


    return hr;
}

//-------------------------------------------------------------------
// Name: DeleteInputStream
//-------------------------------------------------------------------
HRESULT TransformBlur::DeleteInputStream(DWORD dwStreamID)
{
    // This MFT has a fixed number of input streams, so the method is not implemented.
    return E_NOTIMPL;
}

//-------------------------------------------------------------------
// Name: AddInputStreams
//-------------------------------------------------------------------
HRESULT TransformBlur::AddInputStreams(
    DWORD   cStreams,
    DWORD* adwStreamIDs
)
{
    // This MFT has a fixed number of output streams, so the method is not implemented.
    return E_NOTIMPL;
}

//-------------------------------------------------------------------
// Name: GetInputAvailableType
// Description: Return a preferred input type.
//-------------------------------------------------------------------

HRESULT TransformBlur::GetInputAvailableType(
    DWORD           dwInputStreamID,
    DWORD           dwTypeIndex, // 0-based
    IMFMediaType** ppType
)
{
    TRACE((L"GetInputAvailableType (stream = %d, type index = %d)\n", dwInputStreamID, dwTypeIndex));

    AutoLock lock(m_critSec);

    if (ppType == NULL)
    {
        return E_INVALIDARG;
    }

    if (!IsValidInputStream(dwInputStreamID))
    {
        return MF_E_INVALIDSTREAMNUMBER;
    }

    HRESULT hr = S_OK;

    // If the output type is set, return that type as our preferred input type.
    if (this->m_spOutputType)
    {
        if (dwTypeIndex > 0)
        {
            return MF_E_NO_MORE_TYPES;
        }

        *ppType = m_spOutputType;
        (*ppType)->AddRef();
    }
    else
    {
        // The output type is not set. Create a partial media type.
        hr = OnGetPartialType(dwTypeIndex, ppType);
    }
    return hr;
}



//-------------------------------------------------------------------
// Name: GetOutputAvailableType
// Description: Return a preferred output type.
//-------------------------------------------------------------------
HRESULT TransformBlur::GetOutputAvailableType(
    DWORD           dwOutputStreamID,
    DWORD           dwTypeIndex, // 0-based
    IMFMediaType** ppType
)
{
    TRACE((L"GetOutputAvailableType (stream = %d, type index = %d)\n", dwOutputStreamID, dwTypeIndex));

    AutoLock lock(m_critSec);

    if (ppType == NULL)
    {
        return E_INVALIDARG;
    }

    if (!IsValidOutputStream(dwOutputStreamID))
    {
        return MF_E_INVALIDSTREAMNUMBER;
    }

    HRESULT hr = S_OK;

    // If the input type is set, return that type as our preferred output type.
    if (this->m_spInputType)
    {
        if (dwTypeIndex > 0)
        {
            return MF_E_NO_MORE_TYPES;
        }

        *ppType = m_spInputType;
        (*ppType)->AddRef();
    }
    else
    {
        // The input type is not set. Create a partial media type.
        hr = OnGetPartialType(dwTypeIndex, ppType);
    }

    return hr;
}



//-------------------------------------------------------------------
// Name: SetInputType
//-------------------------------------------------------------------

HRESULT TransformBlur::SetInputType(
    DWORD           dwInputStreamID,
    IMFMediaType* pType, // Can be NULL to clear the input type.
    DWORD           dwFlags
)
{
    TRACE((L"TransformBlur::SetInputType\n"));

    AutoLock lock(m_critSec);
    D3DFORMAT d;
    GUID g = GUID_NULL;

    if (!IsValidInputStream(dwInputStreamID))
    {
        return MF_E_INVALIDSTREAMNUMBER;
    }

    // Validate flags.
    if (dwFlags & ~MFT_SET_TYPE_TEST_ONLY)
    {
        return E_INVALIDARG;
    }

    HRESULT hr = S_OK;

    // Does the caller want us to set the type, or just test it?
    BOOL bReallySet = ((dwFlags & MFT_SET_TYPE_TEST_ONLY) == 0);

    // If we have an input sample, the client cannot change the type now.
    if (HasPendingOutput())
    {
        CHECK_HR(hr = MF_E_TRANSFORM_CANNOT_CHANGE_MEDIATYPE_WHILE_PROCESSING);
    }

    // Validate the type, if non-NULL.
    if (pType)
    {
        CHECK_HR(hr = OnCheckInputType(pType));
    }

    // The type is OK. 
    // Set the type, unless the caller was just testing.
    if (bReallySet)
    {
        CHECK_HR(hr = OnSetInputType(pType));
    }

done:

    return hr;
}



//-------------------------------------------------------------------
// Name: SetOutputType
//-------------------------------------------------------------------

HRESULT TransformBlur::SetOutputType(
    DWORD           dwOutputStreamID,
    IMFMediaType* pType, // Can be NULL to clear the output type.
    DWORD           dwFlags
)
{
    TRACE((L"TransformBlur::SetOutputType\n"));

    AutoLock lock(m_critSec);

    if (!IsValidOutputStream(dwOutputStreamID))
    {
        return MF_E_INVALIDSTREAMNUMBER;
    }

    // Validate flags.
    if (dwFlags & ~MFT_SET_TYPE_TEST_ONLY)
    {
        return E_INVALIDARG;
    }

    HRESULT hr = S_OK;


    // Does the caller want us to set the type, or just test it?
    BOOL bReallySet = ((dwFlags & MFT_SET_TYPE_TEST_ONLY) == 0);

    // If we have an input sample, the client cannot change the type now.
    if (HasPendingOutput())
    {
        CHECK_HR(hr = MF_E_TRANSFORM_CANNOT_CHANGE_MEDIATYPE_WHILE_PROCESSING);
    }

    // Validate the type, if non-NULL.
    if (pType)
    {
        CHECK_HR(hr = OnCheckOutputType(pType));
    }

    if (bReallySet)
    {
        // The type is OK. 
        // Set the type, unless the caller was just testing.
        CHECK_HR(hr = OnSetOutputType(pType));
    }

done:
    return hr;
}



//-------------------------------------------------------------------
// Name: GetInputCurrentType
// Description: Returns the current input type.
//-------------------------------------------------------------------

HRESULT TransformBlur::GetInputCurrentType(
    DWORD           dwInputStreamID,
    IMFMediaType** ppType
)
{
    AutoLock lock(m_critSec);

    if (ppType == NULL)
    {
        return E_POINTER;
    }

    if (!IsValidInputStream(dwInputStreamID))
    {
        return MF_E_INVALIDSTREAMNUMBER;
    }

    if (!m_spInputType)
    {
        return MF_E_TRANSFORM_TYPE_NOT_SET;
    }

    *ppType = m_spInputType;
    (*ppType)->AddRef();

    return S_OK;

}



//-------------------------------------------------------------------
// Name: GetOutputCurrentType
// Description: Returns the current output type.
//-------------------------------------------------------------------

HRESULT TransformBlur::GetOutputCurrentType(
    DWORD           dwOutputStreamID,
    IMFMediaType** ppType
)
{
    AutoLock lock(m_critSec);

    if (ppType == NULL)
    {
        return E_POINTER;
    }

    if (!IsValidOutputStream(dwOutputStreamID))
    {
        return MF_E_INVALIDSTREAMNUMBER;
    }

    if (!m_spOutputType)
    {
        return MF_E_TRANSFORM_TYPE_NOT_SET;
    }

    *ppType = m_spOutputType;
    (*ppType)->AddRef();

    return S_OK;

}



//-------------------------------------------------------------------
// Name: GetInputStatus
// Description: Query if the MFT is accepting more input.
//-------------------------------------------------------------------

HRESULT TransformBlur::GetInputStatus(
    DWORD           dwInputStreamID,
    DWORD* pdwFlags
)
{
    TRACE((L"GetInputStatus\n"));

    AutoLock lock(m_critSec);

    if (pdwFlags == NULL)
    {
        return E_POINTER;
    }

    if (!IsValidInputStream(dwInputStreamID))
    {
        return MF_E_INVALIDSTREAMNUMBER;
    }

    // If we already have an input sample, we don't accept
    // another one until the client calls ProcessOutput or Flush.
    if (m_spSample == NULL)
    {
        *pdwFlags = MFT_INPUT_STATUS_ACCEPT_DATA;
    }
    else
    {
        *pdwFlags = 0;
    }

    return S_OK;
}



//-------------------------------------------------------------------
// Name: GetOutputStatus
// Description: Query if the MFT can produce output.
//-------------------------------------------------------------------

HRESULT TransformBlur::GetOutputStatus(DWORD* pdwFlags)
{
    TRACE((L"GetOutputStatus\n"));

    AutoLock lock(m_critSec);

    if (pdwFlags == NULL)
    {
        return E_POINTER;
    }

    // We can produce an output sample if (and only if)
    // we have an input sample.
    if (m_spSample != NULL)
    {
        *pdwFlags = MFT_OUTPUT_STATUS_SAMPLE_READY;
    }
    else
    {
        *pdwFlags = 0;
    }

    return S_OK;
}



//-------------------------------------------------------------------
// Name: SetOutputBounds
// Sets the range of time stamps that the MFT will output.
//-------------------------------------------------------------------

HRESULT TransformBlur::SetOutputBounds(
    LONGLONG        hnsLowerBound,
    LONGLONG        hnsUpperBound
)
{
    // Implementation of this method is optional. 
    return E_NOTIMPL;
}



//-------------------------------------------------------------------
// Name: ProcessEvent
// Sends an event to an input stream.
//-------------------------------------------------------------------

HRESULT TransformBlur::ProcessEvent(
    DWORD              dwInputStreamID,
    IMFMediaEvent* pEvent
)
{
    // This MFT does not handle any stream events, so the method can 
    // return E_NOTIMPL. This tells the pipeline that it can stop 
    // sending any more events to this MFT.
    return E_NOTIMPL;
}



//-------------------------------------------------------------------
// Name: ProcessMessage
//-------------------------------------------------------------------

HRESULT TransformBlur::ProcessMessage(
    MFT_MESSAGE_TYPE    eMessage,
    ULONG_PTR           ulParam
)
{
    AutoLock lock(m_critSec);

    HRESULT hr = S_OK;

    switch (eMessage)
    {
    case MFT_MESSAGE_COMMAND_FLUSH:
        // Flush the MFT.
        hr = OnFlush();
        break;

    case MFT_MESSAGE_COMMAND_DRAIN:
        // Drain: Tells the MFT not to accept any more input until 
        // all of the pending output has been processed. That is our 
        // default behevior already, so there is nothing to do.
        break;

    case MFT_MESSAGE_SET_D3D_MANAGER:
        // The pipeline should never send this message unless the MFT
        // has the MF_SA_D3D_AWARE attribute set to TRUE. However, if we
        // do get this message, it's invalid and we don't implement it.
        hr = OnSetD3DManager(ulParam);
        break;

        // The remaining messages do not require any action from this MFT.
    case MFT_MESSAGE_NOTIFY_BEGIN_STREAMING:
        SetupAlloc();
        break;
    case MFT_MESSAGE_NOTIFY_END_STREAMING:
    case MFT_MESSAGE_NOTIFY_END_OF_STREAM:
    case MFT_MESSAGE_NOTIFY_START_OF_STREAM:
        break;
    }

    return hr;
}

// TODO: Change param to be IUnknown* so can use from different locations
HRESULT TransformBlur::OnSetD3DManager(ULONG_PTR ulParam)
{
    HRESULT hr = S_OK;
    CComPtr<IUnknown> pUnk;
    CComPtr<IMFDXGIDeviceManager> pDXGIDeviceManager;
    if (ulParam != NULL) {
        pUnk = (IUnknown*)ulParam;
        hr = pUnk->QueryInterface(&pDXGIDeviceManager);
        if (SUCCEEDED(hr) && m_spDeviceManager != pDXGIDeviceManager) {
            // Invalidate dx11 resources
            m_spDeviceManager.Release();
            m_spContext.Release();

            // Update dx11 device and resources
            m_spDeviceManager = pDXGIDeviceManager;
            CHECK_HR(hr = m_spDeviceManager->OpenDeviceHandle(&m_hDeviceHandle));
            CHECK_HR(hr = m_spDeviceManager->GetVideoService(m_hDeviceHandle, IID_PPV_ARGS(&m_spDevice)));
            m_spDevice->GetImmediateContext(&m_spContext);

            // Hand off the new device to the sample allocator, if it exists
            if (m_spOutputSampleAllocator)
            {
                CHECK_HR(hr = m_spOutputSampleAllocator->SetDirectXManager(pUnk.p));
            }
        }

        // TODO: Do we know input/output types by now? 

    }
    else 
    {
        // Invalidate dx11 resources
        m_spDeviceManager.Release();
        m_spContext.Release();
    }

done:
    //TODO: Safe release anything as needed
    //SAFE_RELEASE(p_deviceHandle);
    return hr;
}

//-------------------------------------------------------------------
// Name: ProcessInput
// Description: Process an input sample.
//-------------------------------------------------------------------

HRESULT TransformBlur::ProcessInput(
    DWORD               dwInputStreamID,
    IMFSample* pSample,
    DWORD               dwFlags
)
{
    AutoLock lock(m_critSec);
    HRESULT hr = S_OK;

    // TODO: If not null, but no actual new sample from engine? 
    if (pSample == NULL)
    {
        return E_POINTER;
    }

    if (!IsValidInputStream(dwInputStreamID))
    {
        return MF_E_INVALIDSTREAMNUMBER;
    }

    if (dwFlags != 0)
    {
        return E_INVALIDARG; // dwFlags is reserved and must be zero.
    }

    if (!m_spInputType || !m_spOutputType)
    {
        return MF_E_NOTACCEPTING;   // Client must set input and output types.
    }

    if (m_spSample != NULL)
    {
        return MF_E_NOTACCEPTING;   // We already have an input sample.
    }

    CHECK_HR(hr = SetupAlloc()); // Ensure allocator set up

    DWORD dwBufferCount = 0;

    // Validate the number of buffers. There should only be a single buffer to hold the video frame. 
    CHECK_HR(hr = pSample->GetBufferCount(&dwBufferCount));

    if (dwBufferCount == 0)
    {
        CHECK_HR(hr = E_FAIL);
    }
    if (dwBufferCount > 1)
    {
        CHECK_HR(hr = MF_E_SAMPLE_HAS_TOO_MANY_BUFFERS);
    }

    // Cache the sample. We do the actual work in ProcessOutput.
    m_spSample = pSample;
done:
    //pSample->Release();
    return hr;
}



//-------------------------------------------------------------------
// Name: ProcessOutput
// Description: Process an output sample.
//-------------------------------------------------------------------

HRESULT TransformBlur::ProcessOutput(
    DWORD                   dwFlags,
    DWORD                   cOutputBufferCount,
    MFT_OUTPUT_DATA_BUFFER* pOutputSamples, // one per stream
    DWORD* pdwStatus
)
{
    AutoLock lock(m_critSec); // TODO: Move closer to critsec stuff below
    LONGLONG hnsDuration = 0;
    LONGLONG hnsTime = 0;

    // Check input parameters...

    // There are no flags that we accept in this MFT.
    // The only defined flag is MFT_PROCESS_OUTPUT_DISCARD_WHEN_NO_BUFFER. This 
    // flag only applies when the MFT marks an output stream as lazy or optional.
    // However there are no lazy or optional streams on this MFT, so the flag is
    // not valid.
    if (dwFlags != 0)
    {
        return E_INVALIDARG;
    }

    if (pOutputSamples == NULL || pdwStatus == NULL)
    {
        return E_POINTER;
    }

    // Must be exactly one output buffer.
    if (cOutputBufferCount != 1)
    {
        return E_INVALIDARG;
    }

    // It must contain a sample if no device manager to allocate ourselves. 
    if (pOutputSamples[0].pSample == NULL && m_spDeviceManager == nullptr)
    {
        return E_INVALIDARG;
    }

    // If we don't have an input sample, we need some input before
    // we can generate any output.
    if (m_spSample == NULL)
    {
        return MF_E_TRANSFORM_NEED_MORE_INPUT;
    }

    HRESULT hr = S_OK;
    CComPtr<IMFMediaBuffer> pMediaBuffer;
    CComPtr<ID3D11Device> spDevice;
    bool bDeviceLocked = false;

    CHECK_HR(hr = SetupAlloc()); // Ensure allocator is set up

    // We allocate samples when have a dx device
    if (m_spDeviceManager != nullptr)
    {
        CHECK_HR(hr = m_spOutputSampleAllocator->AllocateSample(&(pOutputSamples[0].pSample)));
    }
    // We should have a sample now, whether or not we have a dx device
    if (pOutputSamples[0].pSample == nullptr)
    {
        hr = E_INVALIDARG;
        goto done;
    }

    // Attempt to lock the device if necessary
    if (false && m_spDeviceManager != nullptr)
    {
        CHECK_HR(hr = m_spDeviceManager->LockDevice(m_hDeviceHandle, IID_PPV_ARGS(&spDevice), TRUE));
        bDeviceLocked = true;
    }

    // TODO: Hand over DXGI buffers/ surfaces to onprocessoutput? 
    CHECK_HR(hr = OnProcessOutput(&pOutputSamples[0].pSample));

    // Set status flags.
    pOutputSamples[0].dwStatus = 0;
    *pdwStatus = 0;
 
    // Copy the duration and time stamp from the input sample,
    // if present.
    if (SUCCEEDED(m_spSample->GetSampleDuration(&hnsDuration)))
    {
        CHECK_HR(hr = pOutputSamples[0].pSample->SetSampleDuration(hnsDuration));
    }

    if (SUCCEEDED(m_spSample->GetSampleTime(&hnsTime)))
    {
        CHECK_HR(hr = pOutputSamples[0].pSample->SetSampleTime(hnsTime));
    }
    
    // Always set the output buffer size!
    CHECK_HR(hr = pOutputSamples[0].pSample->GetBufferByIndex(0, &pMediaBuffer));
    CHECK_HR(hr = pMediaBuffer->SetCurrentLength(m_cbImageSize));

done:
    m_spSample.Release(); 
    if (false && bDeviceLocked)
    {
        hr = m_spDeviceManager->UnlockDevice(m_hDeviceHandle, FALSE);
    }
    return hr;
}

/// PRIVATE METHODS
HRESULT TransformBlur::SetupAlloc()
{
    HRESULT hr = S_OK;
    // Fail fast if already set up 
    if (!m_bStreamingInitialized)
    {
        // Check if device the same? 

        // If we have a device manager, we need to set up sample allocator
        if (m_spDeviceManager != nullptr)
        {
            // Set up allocator attributes
            DWORD dwBindFlags = MFGetAttributeUINT32(m_spOutputAttributes.p, MF_SA_D3D11_BINDFLAGS, D3D11_BIND_RENDER_TARGET);
            dwBindFlags |= D3D11_BIND_RENDER_TARGET;        // Must always set as render target! (works but why?) 
            CHECK_HR(hr = m_spOutputAttributes->SetUINT32(MF_SA_D3D11_BINDFLAGS, dwBindFlags));
            CHECK_HR(hr = m_spOutputAttributes->SetUINT32(MF_SA_BUFFERS_PER_SAMPLE, 1));
            CHECK_HR(hr = m_spOutputAttributes->SetUINT32(MF_SA_D3D11_USAGE, D3D11_USAGE_DEFAULT));

            // Set up the output sample allocator if needed
            if (NULL == m_spOutputSampleAllocator)
            {
                CComPtr<IMFVideoSampleAllocatorEx> spVideoSampleAllocator;
                CComPtr<IUnknown> spDXGIManagerUnk;

                CHECK_HR(hr = MFCreateVideoSampleAllocatorEx(IID_PPV_ARGS(&spVideoSampleAllocator)));
                CHECK_HR(hr = m_spDeviceManager->QueryInterface(&spDXGIManagerUnk));

                CHECK_HR(hr = spVideoSampleAllocator->SetDirectXManager(spDXGIManagerUnk));

                m_spOutputSampleAllocator.Attach(spVideoSampleAllocator.Detach());
            }

            CHECK_HR(hr = m_spOutputSampleAllocator->InitializeSampleAllocatorEx(2, 15, m_spOutputAttributes, m_spOutputType));
        }
        m_bStreamingInitialized = true;

    }
done:
    return hr;
}

//-------------------------------------------------------------------
// Name: OnGetPartialType
// Description: Returns a partial media type from our list.
//
// dwTypeIndex: Index into the list of peferred media types.
// ppmt: Receives a pointer to the media type.
//-------------------------------------------------------------------

HRESULT TransformBlur::OnGetPartialType(DWORD dwTypeIndex, IMFMediaType** ppmt)
{
    HRESULT hr = S_OK;

    if (dwTypeIndex >= g_cNumSubtypes)
    {
        return MF_E_NO_MORE_TYPES;
    }

    IMFMediaType* pmt = NULL;

    CHECK_HR(hr = MFCreateMediaType(&pmt));

    CHECK_HR(hr = pmt->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video));

    CHECK_HR(hr = pmt->SetGUID(MF_MT_SUBTYPE, *g_MediaSubtypes[dwTypeIndex]));

    *ppmt = pmt;
    (*ppmt)->AddRef();

done:
    SAFE_RELEASE(pmt);
    return hr;
}


//-------------------------------------------------------------------
// Name: OnCheckInputType
// Description: Validate an input media type.
//-------------------------------------------------------------------

HRESULT TransformBlur::OnCheckInputType(IMFMediaType* pmt)
{
    TRACE((L"OnCheckInputType\n"));
    assert(pmt != NULL);

    HRESULT hr = S_OK;

    // If the output type is set, see if they match.
    if (m_spOutputType != NULL)
    {
        DWORD flags = 0;
        hr = pmt->IsEqual(m_spOutputType, &flags);

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

HRESULT TransformBlur::OnCheckOutputType(IMFMediaType* pmt)
{
    TRACE((L"OnCheckOutputType\n"));
    assert(pmt != NULL);

    HRESULT hr = S_OK;

    // If the input type is set, see if they match.
    if (m_spInputType != NULL)
    {
        DWORD flags = 0;
        hr = pmt->IsEqual(m_spInputType, &flags);

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
HRESULT TransformBlur::OnCheckMediaType(IMFMediaType* pmt)
{
    LogMediaType(pmt);

    GUID major_type = GUID_NULL;
    GUID subtype = GUID_NULL;
    MFVideoInterlaceMode interlace = MFVideoInterlace_Unknown;
    UINT32 val = 0;
    BOOL bFoundMatchingSubtype = FALSE;

    HRESULT hr = S_OK;

    // Major type must be video.
    CHECK_HR(hr = pmt->GetGUID(MF_MT_MAJOR_TYPE, &major_type));

    if (major_type != MFMediaType_Video)
    {
        CHECK_HR(hr = MF_E_INVALIDMEDIATYPE);
    }

    // Subtype must be one of the subtypes in our global list.

    // Get the subtype GUID.
    CHECK_HR(hr = pmt->GetGUID(MF_MT_SUBTYPE, &subtype));

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
        CHECK_HR(hr = MF_E_INVALIDMEDIATYPE);
    }

    // Video must be progressive frames.
    CHECK_HR(hr = pmt->GetUINT32(MF_MT_INTERLACE_MODE, (UINT32*)&interlace));
    if (!(interlace == MFVideoInterlace_Progressive /* || interlace == MFVideoInterlace_MixedInterlaceOrProgressive*/))
    {
        CHECK_HR(hr = MF_E_INVALIDMEDIATYPE);
    }

done:
    return hr;
}


//-------------------------------------------------------------------
// Name: OnSetInputType
// Description: Sets or clears the input media type.
//
// Prerequisite:
// The input type has already been validated.
//-------------------------------------------------------------------

HRESULT TransformBlur::OnSetInputType(IMFMediaType* pmt)
{
    TRACE((L"TransformBlur::OnSetInputType\n"));

    // if pmt is NULL, clear the type.
    // if pmt is non-NULL, set the type.

    m_spInputType.Release();
    m_spInputType = pmt; // TODO: Do we need this to addref, or should we attach? 
    /*if (m_spInputType)
    {
        m_spInputType->AddRef();
    }*/

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

HRESULT TransformBlur::OnSetOutputType(IMFMediaType* pmt)
{
    TRACE((L"TransformBlur::OnSetOutputType\n"));

    // if pmt is NULL, clear the type.
    // if pmt is non-NULL, set the type.

    m_spOutputType.Release();
    m_spOutputType = pmt;
    /*if (m_spOutputType)
    {
        m_spOutputType->AddRef();
    }*/

    return S_OK;
}

HRESULT LockDevice(
    IMFDXGIDeviceManager* pDeviceManager,
    BOOL fBlock,
    ID3D11Device** ppDevice, // Receives a pointer to the device.
    HANDLE* pHandle              // Receives a device handle.   
)
{
    *pHandle = NULL;
    *ppDevice = NULL;

    HANDLE hDevice = 0;

    HRESULT hr = pDeviceManager->OpenDeviceHandle(&hDevice);

    if (SUCCEEDED(hr))
    {
        hr = pDeviceManager->LockDevice(hDevice, IID_PPV_ARGS(ppDevice), fBlock);
    }

    if (hr == DXVA2_E_NEW_VIDEO_DEVICE)
    {
        // Invalid device handle. Try to open a new device handle.
        hr = pDeviceManager->CloseDeviceHandle(hDevice);

        if (SUCCEEDED(hr))
        {
            hr = pDeviceManager->OpenDeviceHandle(&hDevice);
        }

        // Try to lock the device again.
        if (SUCCEEDED(hr))
        {
            hr = pDeviceManager->LockDevice(hDevice, IID_PPV_ARGS(ppDevice), TRUE);
        }
    }

    if (SUCCEEDED(hr))
    {
        *pHandle = hDevice;
    }
    return hr;
}


VideoFrame TransformBlur::SampleToVideoFrame(IMFSample* sample, CComPtr<IVideoFrameNativeFactory> factory)
{
    winrt::com_ptr<IVideoFrameNative> vfNative;
    factory->CreateFromMFSample(sample,
        __uuidof(m_spInputType),
        m_imageWidthInPixels,
        m_imageHeightInPixels,
        true,
        NULL,
        m_spDeviceManager,
        IID_PPV_ARGS(&vfNative)
    );

    return vfNative.try_as<VideoFrame>();
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
IDirect3DSurface TransformBlur::SampleToD3Dsurface(IMFSample* sample)
{
    CComPtr<IMFMediaBuffer> pBuffIn;
    CComPtr<IMFDXGIBuffer> pSrc;
    CComPtr<ID3D11Texture2D> pTextSrc;
    CComPtr<IDXGISurface> pSurfaceSrc;
    
    winrt::com_ptr<IInspectable> pSurfaceInspectable;

    sample->ConvertToContiguousBuffer(&pBuffIn);
    pBuffIn->QueryInterface(IID_PPV_ARGS(&pSrc));
    pSrc->GetResource(IID_PPV_ARGS(&pTextSrc));
    pTextSrc->QueryInterface(IID_PPV_ARGS(&pSurfaceSrc));

    CreateDirect3D11SurfaceFromDXGISurface(pSurfaceSrc, pSurfaceInspectable.put());
    return pSurfaceInspectable.try_as<IDirect3DSurface>();
}
//-------------------------------------------------------------------
// Name: OnProcessOutput
// Description: Generates output data.
// 
//-------------------------------------------------------------------
HRESULT TransformBlur::OnProcessOutput(IMFSample** ppOut)
{
    HRESULT hr = S_OK;

    // Create video frame from input and output samples
    // Pass in video frame to model 
    // Run 
    // Make sure that output gets the correct surface still
    CComPtr<IVideoFrameNativeFactory> factory;
    hr = CoCreateInstance(CLSID_VideoFrameNativeFactory, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory));
    //VideoFrame srcVF = SampleToVideoFrame(m_spSample, factory);
    //VideoFrame destVF = SampleToVideoFrame(*ppOut, factory);

    IDirect3DSurface src = SampleToD3Dsurface(m_spSample);
    IDirect3DSurface dest = SampleToD3Dsurface(*ppOut);

    winrt::com_ptr<ID3D11Texture2D> pTextSrc;
    winrt::com_ptr<ID3D11Texture2D> pTextCreate;
    winrt::com_ptr<ID3D11Texture2D> pTextDest;
    CComPtr<ID3D11Device> deviceSrc;
    CComPtr<ID3D11Device> deviceTemp;
    CComPtr<ID3D11Device> deviceDest;

    auto spDxgiInterfaceAccess = src.as<Windows::Graphics::DirectX::Direct3D11::IDirect3DDxgiInterfaceAccess>();
    hr = spDxgiInterfaceAccess->GetInterface(IID_PPV_ARGS(&pTextSrc));
    pTextSrc->GetDevice(&deviceSrc);

    VideoFrame vfSrc = VideoFrame::CreateWithDirect3D11Surface(src);
    spDxgiInterfaceAccess = vfSrc.Direct3DSurface().as<Windows::Graphics::DirectX::Direct3D11::IDirect3DDxgiInterfaceAccess>();
    hr = spDxgiInterfaceAccess->GetInterface(IID_PPV_ARGS(&pTextCreate));
    pTextCreate->GetDevice(&deviceTemp);

    auto desc = vfSrc.Direct3DSurface().Description();
    VideoFrame vfDesc = VideoFrame::CreateAsDirect3D11SurfaceBacked(desc.Format, desc.Width, desc.Height);
    vfSrc.CopyToAsync(vfDesc).get();
    spDxgiInterfaceAccess = vfDesc.Direct3DSurface().as<Windows::Graphics::DirectX::Direct3D11::IDirect3DDxgiInterfaceAccess>();
    hr = spDxgiInterfaceAccess->GetInterface(IID_PPV_ARGS(&pTextDest));
    pTextCreate->GetDevice(&deviceDest);



    // Invoke the image transform function.
    if (SUCCEEDED(hr))
    {
        // Do the copies inside runtest
        m_segmentModel.RunTestDXGI(src, dest);
    }
    else
    {
        CHECK_HR(hr = E_UNEXPECTED);
    }
    // Always set the output buffer size
    // TODO: Move to ProcessOutput? 
    // CHECK_HR(hr = pBuffOut->SetCurrentLength(m_cbImageSize));

    // The VideoBufferLock class automatically unlocks the buffers.
    // TODO: Does above still apply? 
done:
    return S_OK;
}


//-------------------------------------------------------------------
// Name: OnFlush
// Description: Flush the MFT.
//-------------------------------------------------------------------

HRESULT TransformBlur::OnFlush()
{
    // For this MFT, flushing just means releasing the input sample.
    m_spSample.Release();
    return S_OK;
}




//-------------------------------------------------------------------
// Name: UpdateFormatInfo
// Description: After the input type is set, update our format 
//              information.
//-------------------------------------------------------------------
HRESULT TransformBlur::UpdateFormatInfo()
{
    HRESULT hr = S_OK;

    GUID subtype = GUID_NULL;

    m_imageWidthInPixels = 0;
    m_imageHeightInPixels = 0;
    m_videoFOURCC = 0;
    m_cbImageSize = 0;

    if (m_spInputType != NULL)
    {
        CHECK_HR(hr = m_spInputType->GetGUID(MF_MT_SUBTYPE, &subtype));

        m_videoFOURCC = subtype.Data1;

        CHECK_HR(hr = MFGetAttributeSize(
            m_spInputType,
            MF_MT_FRAME_SIZE,
            &m_imageWidthInPixels,
            &m_imageHeightInPixels
        ));

        TRACE((L"Frame size: %d x %d\n", m_imageWidthInPixels, m_imageHeightInPixels));

        // Calculate the image size (not including padding)
        CHECK_HR(hr = GetImageSize(m_videoFOURCC, m_imageWidthInPixels, m_imageHeightInPixels, &m_cbImageSize));

        // Set the size of the SegmentModel
        m_segmentModel = SegmentModel(m_imageWidthInPixels, m_imageHeightInPixels);
    }
    
done:
    return hr;
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
    case FOURCC_YUY2:
    case FOURCC_UYVY:
        // check overflow
        if ((width > MAXDWORD / 2) ||
            (width * 2 > MAXDWORD / height))
        {
            hr = E_INVALIDARG;
        }
        else
        {
            // 16 bpp
            *pcbImage = width * height * 2;
        }
        break;

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

