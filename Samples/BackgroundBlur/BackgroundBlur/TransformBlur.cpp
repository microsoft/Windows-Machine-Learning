#include "TransformBlur.h"


#define CHECK_HR(hr) if (FAILED(hr)) { goto done; }
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
    //&MEDIASUBTYPE_NV12,
    //&MEDIASUBTYPE_YUY2,
    //&MEDIASUBTYPE_UYVY
};

// Number of media types in the aray.
DWORD g_cNumSubtypes = ARRAY_SIZE(g_MediaSubtypes);

// GetImageSize: Returns the size of a video frame, in bytes.
HRESULT GetImageSize(FOURCC fcc, UINT32 width, UINT32 height, DWORD* pcbImage);
// ConvertMFTypeToDXVAType: Convert an IMFMediaType to DXVA_Desc
HRESULT ConvertMFTypeToDXVAType(IMFMediaType * pType, DXVA2_VideoDesc * pDesc);

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
    m_pSample(NULL),
    m_pInputType(NULL),
    m_pOutputType(NULL),
    m_videoFOURCC(0),
    m_imageWidthInPixels(0),
    m_imageHeightInPixels(0),
    m_cbImageSize(0),
    m_pD3DDeviceManager(NULL),
    m_pHandle(NULL),
    m_pD3DDevice(NULL),
    m_pD3DVideoDevice(NULL)
{
}

//Destructor
TransformBlur::~TransformBlur()
{
    assert(m_nRefCount == 0);

    SAFE_RELEASE(m_pInputType);
    SAFE_RELEASE(m_pOutputType);
    SAFE_RELEASE(m_pSample);

    m_pD3DDeviceManager->CloseDeviceHandle(m_pHandle);
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

    if (m_pInputType == NULL)
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
        MFT_OUTPUT_STREAM_FIXED_SAMPLE_SIZE
        // MFT_OUTPUT_STREAM_PROVIDES_SAMPLES
        ;

    if (m_pOutputType == NULL)
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
HRESULT TransformBlur::GetAttributes(IMFAttributes** pAttributes)
{
    HRESULT hr = MFCreateAttributes(pAttributes, 1);
    hr = (*pAttributes)->SetUINT32(MF_SA_D3D_AWARE, FALSE);
    return hr;
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
    HRESULT hr = MFCreateAttributes(ppAttributes, 1);
    hr = (*ppAttributes)->SetUINT32(MF_SA_MINIMUM_OUTPUT_SAMPLE_COUNT, 3);


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
    if (this->m_pOutputType)
    {
        if (dwTypeIndex > 0)
        {
            return MF_E_NO_MORE_TYPES;
        }

        *ppType = m_pOutputType;
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
    if (this->m_pInputType)
    {
        if (dwTypeIndex > 0)
        {
            return MF_E_NO_MORE_TYPES;
        }

        *ppType = m_pInputType;
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

    // Find a decoder configuration
    if (m_pD3DDeviceManager) 
    {
        UINT numProfiles = m_pD3DVideoDevice->GetVideoDecoderProfileCount();
        for (UINT i = 0; i < numProfiles; i++)
        {
            GUID pDecoderProfile = GUID_NULL;
            CHECK_HR(hr = m_pD3DVideoDevice->GetVideoDecoderProfile(i, &pDecoderProfile));

            BOOL rgbSupport; 
            hr = m_pD3DVideoDevice->CheckVideoDecoderFormat(&pDecoderProfile, DXGI_FORMAT_AYUV, &rgbSupport);
            // DXGI_FORMAT_R8G8B8A8_UNORM or DXGI_FORMAT_B8G8R8X8_UNORM
            hr = m_pD3DVideoDevice->CheckVideoDecoderFormat(&pDecoderProfile, DXGI_FORMAT_B8G8R8X8_UNORM, &rgbSupport);
            if (rgbSupport) {
                // D3D11_DECODER_PROFILE_H264_VLD_NOFGT
                OutputDebugString(L"supports RGB32!\n");
            }

        }
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

    if (!m_pInputType)
    {
        return MF_E_TRANSFORM_TYPE_NOT_SET;
    }

    *ppType = m_pInputType;
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

    if (!m_pOutputType)
    {
        return MF_E_TRANSFORM_TYPE_NOT_SET;
    }

    *ppType = m_pOutputType;
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
    if (m_pSample == NULL)
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
    if (m_pSample != NULL)
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
    // First check if ulParam null, ie. if clearing the d3d manager
    if (ulParam == NULL) 
    {
        m_pD3DDeviceManager.Release();
        return S_OK;
    }

    IMFDXGIDeviceManager* p_IMFDXGIManager = NULL;
    HANDLE p_deviceHandle;

    // TODO: Change video and video device to fields
    ID3D11VideoDevice* pDecoderService = NULL;
    ID3D11Device* pd3dDevice = NULL;

    // Get the Device Manager sent from the  topology loader
    IUnknown* ptr = (IUnknown*) ulParam;

    HRESULT hr = ptr->QueryInterface(IID_IMFDXGIDeviceManager, (void**)&m_pD3DDeviceManager);
    if (FAILED(hr))
    {
        goto done;
    }

    // Get a handle to the DXVA decoder service
    // TODO: Change to give to the field instead of local variable
    hr = m_pD3DDeviceManager->OpenDeviceHandle(&p_deviceHandle);

    // Get the d3d11 device
    m_pD3DDeviceManager->GetVideoService(p_deviceHandle, IID_ID3D12Device, (void**) &m_pD3DDevice);
    
    // Get the d3d11 video device
    hr = m_pD3DDeviceManager->GetVideoService(p_deviceHandle, IID_ID3D11VideoDevice, (void**) &m_pD3DVideoDevice);

    if (FAILED(hr)) 
    {
        OutputDebugString(L"Failed to get DXVA decoder service");
        ProcessMessage(MFT_MESSAGE_SET_D3D_MANAGER, NULL);
    }

    // TOOD: Get the ID3D11DeviceContext and use to add multi-thread protection on the device context

done:
    //TODO: Safe release anything as needed
    SAFE_RELEASE(ptr);
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
    IMFMediaBuffer* pBuffer = NULL;

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

    if (!m_pInputType || !m_pOutputType)
    {
        return MF_E_NOTACCEPTING;   // Client must set input and output types.
    }

    if (m_pSample != NULL)
    {
        return MF_E_NOTACCEPTING;   // We already have an input sample.
    }

    HRESULT hr = S_OK;
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
    m_pSample = pSample;
    
    // Check if for some reason already have d3dsurface
    /*hr = pSample->GetBufferByIndex(0, &pBuffer);
    if (SUCCEEDED(hr))
    {
        HRESULT test = MFGetService(pBuffer, MR_BUFFER_SERVICE, IID_PPV_ARGS(ppSurface));
        pBuffer->Release();
    }*/

    pSample->AddRef();  // Hold a reference count on the sample.

done:
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
    AutoLock lock(m_critSec);
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

    // It must contain a sample.
    if (pOutputSamples[0].pSample == NULL)
    {
        return E_INVALIDARG;
    }

    // If we don't have an input sample, we need some input before
    // we can generate any output.
    if (m_pSample == NULL)
    {
        return MF_E_TRANSFORM_NEED_MORE_INPUT;
    }

    HRESULT hr = S_OK;

    IMFMediaBuffer* pInput = NULL;
    IMFMediaBuffer* pOutput = NULL;

    // Get the input buffer.
    CHECK_HR(hr = m_pSample->ConvertToContiguousBuffer(&pInput));

    // Get the output buffer.
    CHECK_HR(hr = pOutputSamples[0].pSample->ConvertToContiguousBuffer(&pOutput));


    CHECK_HR(hr = OnProcessOutput(pInput, pOutput));
    //m_pSample->CopyToBuffer(pOutput);

    // Set status flags.
    pOutputSamples[0].dwStatus = 0;
    *pdwStatus = 0;


    // Copy the duration and time stamp from the input sample,
    // if present.

    if (SUCCEEDED(m_pSample->GetSampleDuration(&hnsDuration)))
    {
        CHECK_HR(hr = pOutputSamples[0].pSample->SetSampleDuration(hnsDuration));
    }

    if (SUCCEEDED(m_pSample->GetSampleTime(&hnsTime)))
    {
        CHECK_HR(hr = pOutputSamples[0].pSample->SetSampleTime(hnsTime));
    }

done:

    SAFE_RELEASE(m_pSample);   // Release our input sample.
    SAFE_RELEASE(pInput);
    SAFE_RELEASE(pOutput);
    return hr;
}



/// PRIVATE METHODS

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
    if (m_pOutputType != NULL)
    {
        DWORD flags = 0;
        hr = pmt->IsEqual(m_pOutputType, &flags);

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
    if (m_pInputType != NULL)
    {
        DWORD flags = 0;
        hr = pmt->IsEqual(m_pInputType, &flags);

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
    if (interlace != MFVideoInterlace_Progressive)
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

    SAFE_RELEASE(m_pInputType);
    m_pInputType = pmt;
    if (m_pInputType)
    {
        m_pInputType->AddRef();
    }

    DXVA2_VideoDesc des;
    HRESULT hr = ConvertMFTypeToDXVAType(m_pInputType, &des);
    

    // Update the format information.
    UpdateFormatInfo();

    return S_OK;
}

// Fills in the DXVA2_ExtendedFormat structure.
void GetDXVA2ExtendedFormatFromMFMediaType(
    IMFMediaType* pType,
    DXVA2_ExtendedFormat* pFormat
)
{
    // Get the interlace mode.
    MFVideoInterlaceMode interlace =
        (MFVideoInterlaceMode)MFGetAttributeUINT32(
            pType, MF_MT_INTERLACE_MODE, MFVideoInterlace_Unknown
        );

    // The values for interlace mode translate directly, except for mixed 
    // interlace or progressive mode.

    if (interlace == MFVideoInterlace_MixedInterlaceOrProgressive)
    {
        // Default to interleaved fields.
        pFormat->SampleFormat = DXVA2_SampleFieldInterleavedEvenFirst;
    }
    else
    {
        pFormat->SampleFormat = (UINT)interlace;
    }

    // The remaining values translate directly.

    // Use the "no-fail" attribute functions and default to "unknown."

    pFormat->VideoChromaSubsampling = MFGetAttributeUINT32(
        pType, MF_MT_VIDEO_CHROMA_SITING, MFVideoChromaSubsampling_Unknown);

    pFormat->NominalRange = MFGetAttributeUINT32(
        pType, MF_MT_VIDEO_NOMINAL_RANGE, MFNominalRange_Unknown);

    pFormat->VideoTransferMatrix = MFGetAttributeUINT32(
        pType, MF_MT_YUV_MATRIX, MFVideoTransferMatrix_Unknown);

    pFormat->VideoLighting = MFGetAttributeUINT32(
        pType, MF_MT_VIDEO_LIGHTING, MFVideoLighting_Unknown);

    pFormat->VideoPrimaries = MFGetAttributeUINT32(
        pType, MF_MT_VIDEO_PRIMARIES, MFVideoPrimaries_Unknown);

    pFormat->VideoTransferFunction = MFGetAttributeUINT32(
        pType, MF_MT_TRANSFER_FUNCTION, MFVideoTransFunc_Unknown);

}


HRESULT ConvertMFTypeToDXVAType(IMFMediaType* pType, DXVA2_VideoDesc* pDesc)
{
    ZeroMemory(pDesc, sizeof(*pDesc));

    GUID                    subtype = GUID_NULL;
    UINT32                  width = 0;
    UINT32                  height = 0;
    UINT32                  fpsNumerator = 0;
    UINT32                  fpsDenominator = 0;

    // The D3D format is the first DWORD of the subtype GUID.
    HRESULT hr = pType->GetGUID(MF_MT_SUBTYPE, &subtype);
    if (FAILED(hr))
    {
        goto done;
    }

    pDesc->Format = (D3DFORMAT)subtype.Data1;

    // Frame size.
    hr = MFGetAttributeSize(pType, MF_MT_FRAME_SIZE, &width, &height);
    if (FAILED(hr))
    {
        goto done;
    }

    pDesc->SampleWidth = width;
    pDesc->SampleHeight = height;

    // Frame rate.
    hr = MFGetAttributeRatio(pType, MF_MT_FRAME_RATE, &fpsNumerator, &fpsDenominator);
    if (FAILED(hr))
    {
        goto done;
    }

    pDesc->InputSampleFreq.Numerator = fpsNumerator;
    pDesc->InputSampleFreq.Denominator = fpsDenominator;

    // Extended format information.
    GetDXVA2ExtendedFormatFromMFMediaType(pType, &pDesc->SampleFormat);

    // For progressive or single-field types, the output frequency is the same as
    // the input frequency. For interleaved-field types, the output frequency is
    // twice the input frequency.  
    pDesc->OutputFrameFreq = pDesc->InputSampleFreq;

    if ((pDesc->SampleFormat.SampleFormat == DXVA2_SampleFieldInterleavedEvenFirst) ||
        (pDesc->SampleFormat.SampleFormat == DXVA2_SampleFieldInterleavedOddFirst))
    {
        pDesc->OutputFrameFreq.Numerator *= 2;
    }

done:
    return hr;
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

    SAFE_RELEASE(m_pOutputType);
    m_pOutputType = pmt;
    if (m_pOutputType)
    {
        m_pOutputType->AddRef();
    }

    return S_OK;
}




void TransformImage_Stub(
    BYTE* pDest,
    const BYTE* pSrc,
    DWORD cbImageSize,
    SegmentModel    segmentModel
)
{
    segmentModel.RunTest(&pSrc, &pDest, cbImageSize);
    // CopyMemory(pDest, pSrc, cbImageSize);
}


//-------------------------------------------------------------------
// Name: OnProcessOutput
// Description: Generates output data.
//-------------------------------------------------------------------
HRESULT TransformBlur::OnProcessOutput(IMFMediaBuffer* pIn, IMFMediaBuffer* pOut)
{
    HRESULT hr = S_OK;

    BYTE* pDest = NULL;         // Destination buffer.
    LONG lDestStride = 0;       // Destination stride.

    BYTE* pSrc = NULL;          // Source buffer.
    LONG lSrcStride = 0;        // Source stride.

    // Helper objects to lock the buffers.
    VideoBufferLock inputLock(pIn);
    VideoBufferLock outputLock(pOut);


    // Stride if the buffer does not support IMF2DBuffer
    LONG lDefaultStride = 0;

    CHECK_HR(hr = GetDefaultStride(m_pInputType, &lDefaultStride));

    // Lock the input buffer.
    CHECK_HR(hr = inputLock.LockBuffer(lDefaultStride, this->m_imageHeightInPixels, &pSrc, &lSrcStride));

    // Lock the output buffer.
    CHECK_HR(hr = outputLock.LockBuffer(lDefaultStride, m_imageHeightInPixels, &pDest, &lDestStride));

    // Invoke the image transform function.
    if (true)
    {
        TransformImage_Stub(pDest, pSrc, m_cbImageSize, m_segmentModel);
    }
    else
    {
        CHECK_HR(hr = E_UNEXPECTED);
    }

    // Set the data size on the output buffer.
    CHECK_HR(hr = pOut->SetCurrentLength(m_cbImageSize));

    // The VideoBufferLock class automatically unlocks the buffers.
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
    SAFE_RELEASE(m_pSample);
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

    if (m_pInputType != NULL)
    {
        CHECK_HR(hr = m_pInputType->GetGUID(MF_MT_SUBTYPE, &subtype));

        m_videoFOURCC = subtype.Data1;

        CHECK_HR(hr = MFGetAttributeSize(
            m_pInputType,
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

