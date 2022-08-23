#include "pch.h"
#include "TransformAsync.h"
#include <Mfapi.h>


HRESULT TransformAsync::BeginGetEvent(
    IMFAsyncCallback* pCallback,
    IUnknown* punkState)
{
    /***************************************
    ** Since this in an internal function
    ** we know m_eventQueue can never be
    ** nullptr due to InitializeTransform()
    ***************************************/

    return m_eventQueue->BeginGetEvent(pCallback, punkState);
}

HRESULT TransformAsync::EndGetEvent(
    IMFAsyncResult* pResult,
    IMFMediaEvent** ppEvent)
{
    /***************************************
    ** Since this in an internal function
    ** we know m_eventQueue can never be
    ** nullptr due to InitializeTransform()
    ***************************************/

    return m_eventQueue->EndGetEvent(pResult, ppEvent);
}

HRESULT TransformAsync::GetEvent(
    DWORD           dwFlags,
    IMFMediaEvent** ppEvent)
{
    /***************************************
    ** Since this in an internal function
    ** we know m_eventQueue can never be
    ** nullptr due to InitializeTransform()
    ***************************************/

    return m_eventQueue->GetEvent(dwFlags, ppEvent);
}

HRESULT TransformAsync::QueueEvent(
    MediaEventType      met,
    REFGUID             guidExtendedType,
    HRESULT             hrStatus,
    const PROPVARIANT* pvValue)
{
    /***************************************
    ** Since this in an internal function
    ** we know m_eventQueue can never be
    ** nullptr due to InitializeTransform()
    ***************************************/

    return m_eventQueue->QueueEventParamVar(met, guidExtendedType, hrStatus, pvValue);
}