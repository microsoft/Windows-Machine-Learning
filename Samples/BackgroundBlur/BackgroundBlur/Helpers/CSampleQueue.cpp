#include "CSampleQueue.h"

class CSampleQueue::CNode
{
public:
    IMFSample*  pSample;
    CNode*      pNext;

    CNode(void)
    {
        pSample = NULL;
        pNext   = NULL;
    }
    ~CNode(void)
    {
        SAFE_RELEASE(pSample);
        pNext   = NULL;
    }
};

HRESULT CSampleQueue::Create(
    CSampleQueue**   ppQueue)
{
    HRESULT         hr          = S_OK;
    CSampleQueue*   pNewQueue   = NULL;

    do
    {
        if(ppQueue == NULL)
        {
            hr = E_POINTER;
            break;
        }

        pNewQueue = new CSampleQueue();
        if(pNewQueue == NULL)
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

ULONG CSampleQueue::AddRef(void)
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
        if(ppvObject == NULL)
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
            *ppvObject = NULL;
            hr = E_NOINTERFACE;
            break;
        }

        AddRef();
    }while(false);

    return hr;
}

ULONG CSampleQueue::Release(void)
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
    CNode*  pNewNode    = NULL;


    do
    {
        if(pSample == NULL)
        {
            hr = E_POINTER;
            break;
        }

        pNewNode = new CNode();
        if(pNewNode == NULL)
        {
            hr = E_OUTOFMEMORY;
            break;
        }

        pNewNode->pSample           = pSample;
        pNewNode->pSample->AddRef();
        pNewNode->pNext             = NULL;
        InterlockedIncrement(&m_length);

        {
            std::lock_guard<std::recursive_mutex> lock(m_mutex);

            if(m_bAddMarker != FALSE)
            {
                hr = pSample->SetUINT64(TransformAsync_MFSampleExtension_Marker, m_pulMarkerID);
                if(FAILED(hr))
                {
                    break;
                }

                m_pulMarkerID   = 0;
                m_bAddMarker    = FALSE;
            }

            if(IsQueueEmpty() != FALSE)
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
    CNode*  pCurrentNode    = NULL;

    do
    {
        if(ppSample == NULL)
        {
            hr = E_POINTER;
            break;
        }

        *ppSample   = NULL;
        InterlockedDecrement(&m_length);

        {
            std::lock_guard<std::recursive_mutex> lock(m_mutex);

            if(IsQueueEmpty() != FALSE)
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


            if(m_pHead == NULL)
            {
                // This was the last sample in the queue
                m_pTail = NULL;

            }
        }
    }while(false);

    SAFE_DELETE(pCurrentNode);

    return hr;
}

HRESULT CSampleQueue::RemoveAllSamples(void)
{
    HRESULT hr          = S_OK;
    CNode*  pCurrNode   = NULL;
    // Scope the lock
    {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);

        while(IsQueueEmpty() == FALSE)
        {
            pCurrNode = m_pHead;

            m_pHead = m_pHead->pNext;

            delete pCurrNode;
        }

        m_pTail = NULL;
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
        m_bAddMarker    = TRUE;
    }

    return hr;
}

BOOL CSampleQueue::IsQueueEmpty(void)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    return (m_pHead == NULL) ? TRUE : FALSE;
}

ULONG CSampleQueue::GetLength()
{
    return m_length;
}

CSampleQueue::CSampleQueue(void)
{
    m_ulRef         = 1;
    m_pHead         = NULL;
    m_pTail         = NULL;
    m_bAddMarker    = FALSE;
    m_pulMarkerID   = 0;

}

CSampleQueue::~CSampleQueue(void)
{
    RemoveAllSamples();
}