#include "TransformAsync.h"

int g_now;

HRESULT TransformAsync::NotifyRelease() 
{
    HRESULT hr = S_OK;
    const UINT64 currFenceValue = m_fenceValue;
    auto fenceComplete = m_spFence->GetCompletedValue();

    do {
        DWORD dwThreadID;
        // Scheduel a Signal command in the queue
        hr = m_spContext->Signal(m_spFence.get(), currFenceValue);
        if (FAILED(hr))
            break;

        // MVP: Wait until the next signal is done. Later wait for 5 more fence values for better avg.
        // if this is a multiple of 5, then we'll take take the next time and average it out. 
        if (currFenceValue % 30 == 0)
        {
            m_spFence->SetEventOnCompletion(currFenceValue, m_hFenceEvent); // Raise FenceEvent when done
            m_hFrameThread = CreateThread(NULL, 0, FrameThreadProc, m_hFenceEvent, 0, &dwThreadID);
        }

        m_fenceValue = currFenceValue + 1;

    } while (false);

    return hr;
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

    HRESULT hr = S_OK;
    com_ptr<IUnknown> pStateUnk;
    com_ptr<IUnknown> pObjectUnk;
    com_ptr<TransformAsync> pMFT;
    com_ptr<IMFSample> pInputSample;

    do
    {
        //TRACE((L"\n Invoke Thread %d | ", std::hash<std::thread::id>()(std::this_thread::get_id())));

        if (pAsyncResult == NULL)
        {
            hr = E_POINTER;
            break;
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
        hr = pAsyncResult->GetState(pStateUnk.put());
        if (FAILED(hr))
            break;
        hr = pStateUnk->QueryInterface(__uuidof(TransformAsync), pMFT.put_void());
        if (!pMFT)
        {
            hr = E_NOINTERFACE;
            break;
        }


        // Now that scheduler has told us to process this sample, call SubmitEval with next
        // Available set of bindings, session. 
        hr = pMFT->SubmitEval(pInputSample.get());
        hr = pAsyncResult->SetStatus(hr);
    } while (false);

    return hr;
}