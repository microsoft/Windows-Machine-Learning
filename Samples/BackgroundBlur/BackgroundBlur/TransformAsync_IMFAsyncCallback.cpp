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

    do
    {
        TRACE((L"\n Invoke Thread %d | ", std::hash<std::thread::id>()(std::this_thread::get_id())));

        if (pAsyncResult == NULL)
        {
            hr = E_POINTER;
            return hr;
        }

        // Get the IMFSample tied to this specific Invoke call
        hr = pAsyncResult->GetObjectW(pObjectUnk.put());
        if (FAILED(hr))
            break;
        pInputSample = pObjectUnk.try_as<IMFSample>();
        if (!pInputSample)
        {
            hr = E_NOINTERFACE;
            break;
        }

        // Get the pMFT current state
        // TODO: pStateUnk isn't a TransformAsync>? 
        hr = pAsyncResult->GetState(pStateUnk.put());
        if (FAILED(hr))
            break;
        hr = pStateUnk->QueryInterface(__uuidof(TransformAsync), pMFT.put_void());
        //pMFT = pStateUnk.try_as<TransformAsync>();
        if (!pMFT)
        {
            hr = E_NOINTERFACE;
            break;
        }


        // Now that scheduler has told us to process this sample, call SubmitEval with next
        // Available set of bindings, session. 
        //std::thread eval(&TransformAsync::SubmitEval, pMFT, pInputSample.get());  // TODO: Change submiteval to have winrt::com_ptr
        hr = pMFT->SubmitEval(pInputSample.get());
        hr = pAsyncResult->SetStatus(hr);
        // TOOD: finish 
    } while (false);

    // TODO: Do I need to call some EndX method here? https://docs.microsoft.com/en-us/windows/win32/medfound/implementing-the-asynchronous-callback
    // So maybe already running and need a Begin to start processing? Then Invoke says when to get the output? 
    return hr;
}