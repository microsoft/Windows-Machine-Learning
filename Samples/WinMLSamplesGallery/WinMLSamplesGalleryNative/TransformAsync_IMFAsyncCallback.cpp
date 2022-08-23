#include "pch.h"
#include "TransformAsync.h"

HRESULT TransformAsync::GetParameters(
    DWORD* pdwFlags,
    DWORD* pdwQueue)
{
    HRESULT hr = S_OK;

    if ((pdwFlags == nullptr) || (pdwQueue == nullptr))
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

    if (pAsyncResult == nullptr)
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