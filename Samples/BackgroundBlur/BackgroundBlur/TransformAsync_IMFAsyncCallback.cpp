#include "TransformAsync.h"
#include <format>

int g_now;

HRESULT TransformAsync::NotifyRelease() 
{
    const UINT64 currFenceValue = m_fenceValue;
    auto fenceComplete = m_fence->GetCompletedValue();
    DWORD dwThreadID;

    // Fail fast if context doesn't exist anymore. 
    if (m_context == nullptr)
    {
        return S_OK;
    }

    // Scheduel a Signal command in the queue
    RETURN_IF_FAILED(m_context->Signal(m_fence.get(), currFenceValue));

    if (currFenceValue % FRAME_RATE_UPDATE == 0)
    {

        m_fence->SetEventOnCompletion(currFenceValue, m_fenceEvent.get()); // Raise FenceEvent when done
        m_frameThread.reset(CreateThread(NULL, 0, FrameThreadProc, m_fenceEvent.get(), 0, &dwThreadID));
    }

    m_fenceValue = currFenceValue + 1;
    return S_OK;
}

HRESULT TransformAsync::GetParameters(
    DWORD* pdwFlags,
    DWORD* pdwQueue)
{
    HRESULT hr = S_OK;

    if ((pdwFlags == NULL) || (pdwQueue == NULL))
    {
        hr = E_POINTER;
        return hr;
    }

    (*pdwFlags) = 0;
    (*pdwQueue) = MFASYNC_CALLBACK_QUEUE_MULTITHREADED;

    return hr;
}

HRESULT TransformAsync::Invoke(
    IMFAsyncResult* pAsyncResult)
{
    /*********************************
    ** Todo: This function is called
    ** when you schedule an async event
    ** Determine the event type from
    ** the result and take appropriate
    ** action
    *********************************/

    com_ptr<IUnknown> stateUnk;
    com_ptr<IUnknown> objectUnk;
    com_ptr<TransformAsync> transform;
    com_ptr<IMFSample> inputSample;

    if (pAsyncResult == NULL)
    {
        return E_POINTER;
    }

    // Get the IMFSample tied to this specific Invoke call
    RETURN_IF_FAILED(pAsyncResult->GetObjectW(objectUnk.put()));
    
    inputSample = objectUnk.try_as<IMFSample>();
    RETURN_IF_NULL_ALLOC(inputSample);

    // Get the transform current state
    RETURN_IF_FAILED(pAsyncResult->GetState(stateUnk.put()));  
    RETURN_IF_FAILED(stateUnk->QueryInterface(__uuidof(TransformAsync), transform.put_void()));

    // Now that scheduler has told us to process this sample, call SubmitEval with next
    // Available set of bindings, session. 
    HRESULT eval = transform->SubmitEval(inputSample.get());
    RETURN_IF_FAILED(pAsyncResult->SetStatus(eval));

    return S_OK;
}