#include "TransformAsync.h"

HRESULT TransformAsync::GetParameters(
    DWORD* pdwFlags,
    DWORD* pdwQueue)
{
    HRESULT hr = S_OK;

    do
    {
        if ((pdwFlags == NULL) || (pdwQueue == NULL))
        {
            hr = E_POINTER;
            break;
        }

        (*pdwFlags) = 0;
        (*pdwQueue) = m_dwDecodeWorkQueueID;
    } while (false);

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

    HRESULT hr = S_OK;
    winrt::com_ptr<IUnknown> pStateUnk;
    winrt::com_ptr<IUnknown> pObjectUnk;
    winrt::com_ptr<TransformAsync> pMFT;
    winrt::com_ptr<IMFSample> pInputSample;

    if (pAsyncResult == NULL)
    {
        hr = E_POINTER;
        return hr;
    }

    // Get the pMFT current state
    CHECK_HR(hr = pAsyncResult->GetState(pStateUnk.put()));
    pMFT = pStateUnk.try_as<TransformAsync>();
    if (!pMFT)
    {
        hr = E_NOINTERFACE;
        return hr;
    }

    // Get the IMFSample tied to this specific Invoke call
    CHECK_HR(hr = pAsyncResult->GetObjectW(pObjectUnk.put()));
    pInputSample.try_as<IMFSample>();
    if (!pInputSample)
    {
        hr = E_NOINTERFACE;
        return hr;
    }

    // Now that scheduler has told us to process this sample, call SubmitEval with next
    // Available set of bindings, session. 
    hr = pMFT->SubmitEval(pInputSample.get());

    // hr = pAsyncResult->SetStatus(hr)
    // TOOD: finish 

done:

    return hr;
}