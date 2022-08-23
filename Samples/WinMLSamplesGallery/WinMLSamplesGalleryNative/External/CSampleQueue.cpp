#include "pch.h"
#include "CSampleQueue.h"

class CSampleQueue::CNode
{
public:
    IMFSample*  pSample;
    CNode*      pNext;

    CNode()
    {
        pSample = nullptr;
        pNext   = nullptr;
    }
    ~CNode()
    {
        SAFE_RELEASE(pSample);
        pNext   = nullptr;
    }
};

HRESULT CSampleQueue::Create(
    CSampleQueue**   ppQueue)
{
    HRESULT         hr          = S_OK;
    CSampleQueue*   pNewQueue   = nullptr;

    do
    {
        if(ppQueue == nullptr)
        {
            hr = E_POINTER;
            break;
        }

        pNewQueue = new CSampleQueue();
        if(pNewQueue == nullptr)
        {
            hr = E_OUTOFMEMORY;
            break;
        }

        (*ppQueue) = pNewQueue;
        (*ppQueue)->AddRef();
    }while(false);

    SAFE_RELEASE(pNewQueue);

    return hr;
}

ULONG CSampleQueue::AddRef()
{
    return InterlockedIncrement(&m_ulRef);
}

HRESULT CSampleQueue::QueryInterface(
    REFIID riid,
    void** ppvObject)
{
    HRESULT hr = S_OK;

    do
    {
        if(ppvObject == nullptr)
        {
            hr = E_POINTER;
            break;
        }

        if(riid == IID_IUnknown)
        {
            *ppvObject = (IUnknown*)this;
        }
        else
        {
            *ppvObject = nullptr;
            hr = E_NOINTERFACE;
            break;
        }

        AddRef();
    }while(false);

    return hr;
}

ULONG CSampleQueue::Release()
{
    ULONG   ulRef = 0;
    
    if(m_ulRef > 0)
    {
        ulRef = InterlockedDecrement(&m_ulRef);
    }

    if(ulRef == 0)
    {
        delete this;
    }

    return ulRef;
}

HRESULT CSampleQueue::AddSample(
    IMFSample*  pSample)
{
    HRESULT hr          = S_OK;
    CNode*  pNewNode    = nullptr;


    do
    {
        if(pSample == nullptr)
        {
            hr = E_POINTER;
            break;
        }

        pNewNode = new CNode();
        if(pNewNode == nullptr)
        {
            hr = E_OUTOFMEMORY;
            break;
        }

        pNewNode->pSample           = pSample;
        pNewNode->pSample->AddRef();
        pNewNode->pNext             = nullptr;
        InterlockedIncrement(&m_length);

        {
            std::lock_guard<std::recursive_mutex> lock(m_mutex);

            if(m_bAddMarker != false)
            {
                hr = pSample->SetUINT64(TransformAsync_MFSampleExtension_Marker, m_pulMarkerID);
                if(FAILED(hr))
                {
                    break;
                }

                m_pulMarkerID   = 0;
                m_bAddMarker    = false;
            }

            if(IsQueueEmpty() != false)
            {
                // This is the first sample in the queue
                m_pHead     = pNewNode;
                m_pTail     = pNewNode;

            }
            else
            {
                m_pTail->pNext  = pNewNode;
                m_pTail         = pNewNode;

            }

        }
    }while(false);

    if(FAILED(hr))
    {
        SAFE_DELETE(pNewNode);
    }


    return hr;
}
 
HRESULT CSampleQueue::GetNextSample(
    IMFSample** ppSample)
{
    HRESULT hr              = S_OK;
    CNode*  pCurrentNode    = nullptr;

    do
    {
        if(ppSample == nullptr)
        {
            hr = E_POINTER;
            break;
        }

        *ppSample   = nullptr;
        InterlockedDecrement(&m_length);

        {
            std::lock_guard<std::recursive_mutex> lock(m_mutex);

            if(IsQueueEmpty() != false)
            {
                // The queue is empty
                hr = S_FALSE;
                break;
            }
            
            pCurrentNode = m_pHead;

            (*ppSample) = pCurrentNode->pSample;
            (*ppSample)->AddRef();

            // Now remove the node from the list

            m_pHead = m_pHead->pNext;


            if(m_pHead == nullptr)
            {
                // This was the last sample in the queue
                m_pTail = nullptr;

            }
        }
    }while(false);

    SAFE_DELETE(pCurrentNode);

    return hr;
}

HRESULT CSampleQueue::RemoveAllSamples()
{
    HRESULT hr          = S_OK;
    CNode*  pCurrNode   = nullptr;
    // Scope the lock
    {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);

        while(IsQueueEmpty() == false)
        {
            pCurrNode = m_pHead;

            m_pHead = m_pHead->pNext;

            delete pCurrNode;
        }

        m_pTail = nullptr;
        m_length = 0;
    }

    return hr;
}

HRESULT CSampleQueue::MarkerNextSample(
    const ULONG_PTR pulID)
{
    HRESULT hr = S_OK;
    // Scope the lock
    {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);

        m_pulMarkerID   = pulID;
        m_bAddMarker    = true;
    }

    return hr;
}

bool CSampleQueue::IsQueueEmpty()
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    return (m_pHead == nullptr) ? true : false;
}

ULONG CSampleQueue::GetLength()
{
    return m_length;
}

CSampleQueue::CSampleQueue()
{
    m_ulRef         = 1;
    m_pHead         = nullptr;
    m_pTail         = nullptr;
    m_bAddMarker    = false;
    m_pulMarkerID   = 0;

}

CSampleQueue::~CSampleQueue()
{
    RemoveAllSamples();
    //m_critSec.~CritSec();
}