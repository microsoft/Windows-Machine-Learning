#include "TransformAsync.h"
#include <mferror.h>
#include <Mfapi.h>

#include <DXGItype.h>
#include <dxgi1_2.h>
#include <dxgi1_3.h>

interface DECLSPEC_UUID("9f251514-9d4d-4902-9d60-18988ab7d4b5") DECLSPEC_NOVTABLE
    IDXGraphicsAnalysis : public IUnknown
{
    STDMETHOD_(void, BeginCapture)() PURE;
    STDMETHOD_(void, EndCapture)() PURE;
};
IDXGraphicsAnalysis* graphicsAnalysis;

//-------------------------------------------------------------------
// Name: GetStreamLimits
// Returns the minimum and maximum number of streams.
//-------------------------------------------------------------------
HRESULT TransformAsync::GetStreamLimits(
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
HRESULT TransformAsync::GetStreamCount(
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
HRESULT TransformAsync::GetStreamIDs(
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
HRESULT TransformAsync::GetInputStreamInfo(
    DWORD                     dwInputStreamID,
    MFT_INPUT_STREAM_INFO* pStreamInfo
)
{
    TRACE((L"GetInputStreamInfo\n"));

    std::lock_guard<std::recursive_mutex> lock(m_mutex);

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
    pStreamInfo->dwFlags = MFT_INPUT_STREAM_WHOLE_SAMPLES
        | MFT_INPUT_STREAM_SINGLE_SAMPLE_PER_BUFFER;

    if (m_inputType == NULL)
    {
        pStreamInfo->cbSize = 0;
    }
    else
    {
        pStreamInfo->cbSize = m_imageSize;
    }

    pStreamInfo->cbMaxLookahead = 0;
    pStreamInfo->cbAlignment = 0;

    return S_OK;
}

//-------------------------------------------------------------------
// Name: GetOutputStreamInfo
// Returns information about an output stream. 
//-------------------------------------------------------------------
HRESULT TransformAsync::GetOutputStreamInfo(
    DWORD                     dwOutputStreamID,
    MFT_OUTPUT_STREAM_INFO* pStreamInfo
)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

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
        MFT_OUTPUT_STREAM_FIXED_SAMPLE_SIZE |
        MFT_OUTPUT_STREAM_PROVIDES_SAMPLES
        ;

    if (m_outputType == NULL)
    {
        pStreamInfo->cbSize = 0;
    }
    else
    {
        pStreamInfo->cbSize = m_imageSize;
    }

    pStreamInfo->cbAlignment = 0;

    return S_OK;
}

/*
  Acquires lock on m_critSec so can't modify MFT attributes concurrently.   
*/
HRESULT TransformAsync::GetAttributes(IMFAttributes** ppAttributes)
{
    if (ppAttributes == NULL)
    {
        return E_POINTER;
    }

    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    *ppAttributes = m_attributes.get();
    if ((*ppAttributes) == NULL)
    {
        return E_UNEXPECTED;
    }
    (*ppAttributes)->AddRef();

    return S_OK;
}


//-------------------------------------------------------------------
// Name: GetInputStreamAttributes
// Returns stream-level attributes for an input stream.
//-------------------------------------------------------------------
HRESULT TransformAsync::GetInputStreamAttributes(
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
HRESULT TransformAsync::GetOutputStreamAttributes(
    DWORD           dwOutputStreamID,
    IMFAttributes** ppAttributes
)
{
    return E_NOTIMPL;
}

//-------------------------------------------------------------------
// Name: DeleteInputStream
//-------------------------------------------------------------------
HRESULT TransformAsync::DeleteInputStream(DWORD dwStreamID)
{
    // This MFT has a fixed number of input streams, so the method is not implemented.
    return E_NOTIMPL;
}

//-------------------------------------------------------------------
// Name: AddInputStreams
//-------------------------------------------------------------------
HRESULT TransformAsync::AddInputStreams(
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

HRESULT TransformAsync::GetInputAvailableType(
    DWORD           dwInputStreamID,
    DWORD           dwTypeIndex, // 0-based
    IMFMediaType** ppType
)
{
    TRACE((L"GetInputAvailableType (stream = %d, type index = %d)\n", dwInputStreamID, dwTypeIndex));

    std::lock_guard<std::recursive_mutex> lock(m_mutex);

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
    if (this->m_outputType)
    {
        if (dwTypeIndex > 0)
        {
            return MF_E_NO_MORE_TYPES;
        }

        *ppType = m_outputType.get();
        (*ppType)->AddRef();
    }
    else
    {
        // The output type is not set. Create a partial media type.
        RETURN_IF_FAILED(OnGetPartialType(dwTypeIndex, ppType));
    }
    return hr;
}

//-------------------------------------------------------------------
// Name: GetOutputAvailableType
// Description: Return a preferred output type.
//-------------------------------------------------------------------
HRESULT TransformAsync::GetOutputAvailableType(
    DWORD           dwOutputStreamID,
    DWORD           dwTypeIndex, // 0-based
    IMFMediaType** ppType
)
{
    TRACE((L"GetOutputAvailableType (stream = %d, type index = %d)\n", dwOutputStreamID, dwTypeIndex));

    std::lock_guard<std::recursive_mutex> lock(m_mutex);

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
    if (this->m_inputType)
    {
        if (dwTypeIndex > 0)
        {
            return MF_E_NO_MORE_TYPES;
        }

        *ppType = m_inputType.get();
        (*ppType)->AddRef();
    }
    else
    {
        // The input type is not set. Create a partial media type.
        RETURN_IF_FAILED(OnGetPartialType(dwTypeIndex, ppType));
    }

    return hr;
}

//-------------------------------------------------------------------
// Name: SetInputType
//-------------------------------------------------------------------

HRESULT TransformAsync::SetInputType(
    DWORD           dwInputStreamID,
    IMFMediaType* pType, // Can be NULL to clear the input type.
    DWORD           dwFlags
)
{
    TRACE((L"TransformAsync::SetInputType\n"));

    std::lock_guard<std::recursive_mutex> lock(m_mutex);
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

    // Does the caller want us to set the type, or just test it?
    BOOL reallySet = ((dwFlags & MFT_SET_TYPE_TEST_ONLY) == 0);

    // Validate the type, if non-NULL.
    if (pType)
    {
        RETURN_IF_FAILED(OnCheckInputType(pType));
    }

    // The type is OK. 
    // Set the type, unless the caller was just testing.
    if (reallySet)
    {
        RETURN_IF_FAILED(OnSetInputType(pType));
    }

    return S_OK;
}


HRESULT TransformAsync::SetOutputType(
    DWORD           dwOutputStreamID,
    IMFMediaType* pType, // Can be NULL to clear the output type.
    DWORD           dwFlags
)
{
    TRACE((L"TransformAsync::SetOutputType\n"));

    if (pType == NULL) 
    {
        return E_POINTER;
    }
    if (!IsValidOutputStream(dwOutputStreamID))
    {
        return MF_E_INVALIDSTREAMNUMBER;
    }

    // Validate flags.
    if (dwFlags & ~MFT_SET_TYPE_TEST_ONLY)
    {
        return E_INVALIDARG;
    }


    // Does the caller want us to set the type, or just test it?
    BOOL reallySet = ((dwFlags & MFT_SET_TYPE_TEST_ONLY) == 0);

    // Validate the type, if non-NULL.
    if (pType)
    {
        RETURN_IF_FAILED(OnCheckOutputType(pType));
    }

    if (reallySet)
    {
        std::lock_guard<std::recursive_mutex> lock(m_mutex); 
        // The type is OK. 
        // Set the type, unless the caller was just testing.
        RETURN_IF_FAILED(OnSetOutputType(pType));
    }

    return S_OK;
}


//-------------------------------------------------------------------
// Name: GetInputCurrentType
// Description: Returns the current input type.
//-------------------------------------------------------------------
HRESULT TransformAsync::GetInputCurrentType(
    DWORD           dwInputStreamID,
    IMFMediaType** ppType
)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    if (ppType == NULL)
    {
        return E_POINTER;
    }

    if (!IsValidInputStream(dwInputStreamID))
    {
        return MF_E_INVALIDSTREAMNUMBER;
    }

    if (!m_inputType)
    {
        return MF_E_TRANSFORM_TYPE_NOT_SET;
    }

    *ppType = m_inputType.get();
    (*ppType)->AddRef();

    return S_OK;

}

//-------------------------------------------------------------------
// Name: GetOutputCurrentType
// Description: Returns the current output type.
//-------------------------------------------------------------------

HRESULT TransformAsync::GetOutputCurrentType(
    DWORD           dwOutputStreamID,
    IMFMediaType** ppType
)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    if (ppType == NULL)
    {
        return E_POINTER;
    }

    if (!IsValidOutputStream(dwOutputStreamID))
    {
        return MF_E_INVALIDSTREAMNUMBER;
    }

    if (!m_outputType)
    {
        return MF_E_TRANSFORM_TYPE_NOT_SET;
    }

    *ppType = m_outputType.get();
    (*ppType)->AddRef();

    return S_OK;

}


//-------------------------------------------------------------------
// Name: GetInputStatus
// Description: Query if the MFT is accepting more input.
//-------------------------------------------------------------------
HRESULT TransformAsync::GetInputStatus(
    DWORD           dwInputStreamID,
    DWORD* pdwFlags
)
{
    TRACE((L"GetInputStatus\n"));

    if (pdwFlags == NULL)
    {
        return E_POINTER;
    }

    if (!IsValidInputStream(dwInputStreamID))
    {
        return MF_E_INVALIDSTREAMNUMBER;
    }

    *pdwFlags = 0;
    {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);

        if ((m_status & MYMFT_STATUS_INPUT_ACCEPT_DATA) != 0)
        {
            *pdwFlags = MFT_INPUT_STATUS_ACCEPT_DATA;
        }
    }
    return S_OK;
}

//-------------------------------------------------------------------
// Name: GetOutputStatus
// Description: Query if the MFT can produce output.
//-------------------------------------------------------------------

HRESULT TransformAsync::GetOutputStatus(DWORD* pdwFlags)
{
    TRACE((L"GetOutputStatus\n"));


    if (pdwFlags == NULL)
    {
        return E_POINTER;
    }

    (*pdwFlags) = 0;
    {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        if ((m_status & MYMFT_STATUS_OUTPUT_SAMPLE_READY) != 0)
        {
            *pdwFlags = MFT_OUTPUT_STATUS_SAMPLE_READY;
        }
    }

    return S_OK;
}



//-------------------------------------------------------------------
// Name: SetOutputBounds
// Sets the range of time stamps that the MFT will output.
//-------------------------------------------------------------------

HRESULT TransformAsync::SetOutputBounds(
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

HRESULT TransformAsync::ProcessEvent(
    DWORD              dwInputStreamID,
    IMFMediaEvent* pEvent
)
{
    // This MFT does not handle any stream events, so the method can 
    // return S_OK. 
    return S_OK;

}

//-------------------------------------------------------------------
// Name: ProcessMessage
//-------------------------------------------------------------------
HRESULT TransformAsync::ProcessMessage(
    MFT_MESSAGE_TYPE    eMessage,
    ULONG_PTR           ulParam
)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    HRESULT hr = S_OK;

    switch (eMessage)
    {
    case MFT_MESSAGE_COMMAND_FLUSH:
    {
        RETURN_IF_FAILED(OnFlush());
        if (FAILED(hr))
        {
            break;
        }
    }
    break;

    case MFT_MESSAGE_COMMAND_DRAIN:
    {
        RETURN_IF_FAILED(OnDrain((UINT32)ulParam));
        if (FAILED(hr))
        {
            break;
        }
    }
    break;

    case MFT_MESSAGE_SET_D3D_MANAGER:
        RETURN_IF_FAILED(OnSetD3DManager(ulParam));
        break;
    case MFT_MESSAGE_NOTIFY_END_OF_STREAM:
    case MFT_MESSAGE_NOTIFY_END_STREAMING:
    {
        RETURN_IF_FAILED(OnEndOfStream());
        if (FAILED(hr))
        {
            break;
        }
    }
    break;
    case MFT_MESSAGE_COMMAND_MARKER:
    {
        RETURN_IF_FAILED(OnMarker(ulParam));
        if (FAILED(hr))
        {
            break;
        }
    }
    break;
    case MFT_MESSAGE_NOTIFY_START_OF_STREAM:
    {
        RETURN_IF_FAILED(OnStartOfStream());
        if (FAILED(hr))
        {
            break;
        }
    }
    break;
    case MFT_MESSAGE_NOTIFY_BEGIN_STREAMING:
    {
        HRESULT getAnalysis = DXGIGetDebugInterface1(0, __uuidof(graphicsAnalysis), reinterpret_cast<void**>(&graphicsAnalysis));
        RETURN_IF_FAILED(SetupAlloc());
        break;
    }
    default:
        break;
    }

    return hr;
}

HRESULT TransformAsync::ProcessOutput(
    DWORD                   dwFlags,
    DWORD                   dwOutputBufferCount,
    MFT_OUTPUT_DATA_BUFFER* pOutputSamples,
    DWORD* pdwStatus)
{
    com_ptr<IMFSample> sample;

    {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        if (m_haveOutputCount == 0)
        {
            // This call does not correspond to a have output call
            return E_UNEXPECTED;
        }
        else
        {
            m_haveOutputCount--;
        }
    }
    if (IsMFTReady() == FALSE)
    {
        return MF_E_TRANSFORM_TYPE_NOT_SET;
    }

    /***************************************
        ** Since this in an internal function
        ** we know m_outputSampleQueue can never be
        ** NULL due to InitializeTransform()
        ***************************************/
    RETURN_IF_FAILED(m_outputSampleQueue->GetNextSample(sample.put()));

    if (sample == NULL)
    {
        return MF_E_TRANSFORM_NEED_MORE_INPUT;
    }

    pOutputSamples[0].dwStreamID = 0;
    if ((pOutputSamples[0].pSample) == NULL)
    {
        // The MFT is providing it's own samples
        (pOutputSamples[0].pSample) = sample.get();
        (pOutputSamples[0].pSample)->AddRef();
    }
    else
    {
        // TODO: pipeline has allocated samples 
    }

    /***************************************
        ** Since this in an internal function
        ** we know m_outputSampleQueue can never be
        ** NULL due to InitializeTransform()
        ***************************************/
    if (m_outputSampleQueue->IsQueueEmpty() != FALSE)
    {
        // We're out of samples in the output queue
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        if ((m_status & MYMFT_STATUS_DRAINING) != 0)
        {
            // We're done draining, time to send the event
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
            ** we know m_eventQueue can never be
            ** NULL due to InitializeTransform()
            ***************************************/
            RETURN_IF_FAILED(m_eventQueue->QueueEvent(pDrainCompleteEvent.get()));

            m_status &= (~MYMFT_STATUS_DRAINING);
        }
    }
    return S_OK;
}

HRESULT TransformAsync::ProcessInput(
    DWORD       dwInputStreamID,
    IMFSample* pSample,
    DWORD       dwFlags)
{
    DWORD currFrameLocal = 0;
    TRACE((L" | PI Thread %d | ", std::hash<std::thread::id>()(std::this_thread::get_id())));

    {
        //graphicsAnalysis->BeginCapture();
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        if (m_needInputCount == 0)
        {
            // This call does not correspond to a need input call
            return MF_E_NOTACCEPTING;
        }
        else
        {
            m_needInputCount--;
        }
        currFrameLocal = m_currFrameNumber++;
    }
    if (pSample == NULL)
    {
        return E_POINTER;
    }

    /*****************************************
        ** Note: If your MFT supports more than one
        ** stream, make sure you modify
        ** MFT_MAX_STREAMS and adjust this function
        ** accordingly
        *****************************************/
    if (dwInputStreamID >= 1)
    {
        return MF_E_INVALIDSTREAMNUMBER;
    }

    // First, put sample into the input Queue

    /***************************************
    ** Since this in an internal function
    ** we know m_inputSampleQueue can never be
    ** NULL due to InitializeTransform()
    ***************************************/
    RETURN_IF_FAILED(m_inputSampleQueue->AddSample(pSample));

    // Now schedule the work to decode the sample
    RETURN_IF_FAILED(ScheduleFrameInference());

    // If not at the max number of Need Input count, fire off another request
    // If we're less than MAX_NUM_INPUT_SAMPLES ahead of the number of process samples, request another
    if ((currFrameLocal - m_processedFrameNum) < m_numThreads)
    {
        RETURN_IF_FAILED(RequestSample(0));
    }
    return S_OK;
}
