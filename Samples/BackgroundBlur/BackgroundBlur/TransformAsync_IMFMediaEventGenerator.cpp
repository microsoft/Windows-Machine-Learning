#include "TransformAsync.h"
#include <Mfapi.h>


HRESULT TransformAsync::BeginGetEvent(
    IMFAsyncCallback* pCallback,
    IUnknown* punkState)
{
    /***************************************
    ** Since this in an internal function
    ** we know m_spEventQueue can never be
    ** NULL due to InitializeTransform()
    ***************************************/

    return m_spEventQueue->BeginGetEvent(pCallback, punkState);
}

HRESULT TransformAsync::EndGetEvent(
    IMFAsyncResult* pResult,
    IMFMediaEvent** ppEvent)
{
    /***************************************
    ** Since this in an internal function
    ** we know m_spEventQueue can never be
    ** NULL due to InitializeTransform()
    ***************************************/

    return m_spEventQueue->EndGetEvent(pResult, ppEvent);
}

HRESULT TransformAsync::GetEvent(
    DWORD           dwFlags,
    IMFMediaEvent** ppEvent)
{
    /***************************************
    ** Since this in an internal function
    ** we know m_spEventQueue can never be
    ** NULL due to InitializeTransform()
    ***************************************/

    return m_spEventQueue->GetEvent(dwFlags, ppEvent);
}

HRESULT TransformAsync::QueueEvent(
    MediaEventType      met,
    REFGUID             guidExtendedType,
    HRESULT             hrStatus,
    const PROPVARIANT* pvValue)
{
    /***************************************
    ** Since this in an internal function
    ** we know m_spEventQueue can never be
    ** NULL due to InitializeTransform()
    ***************************************/

    return m_spEventQueue->QueueEventParamVar(met, guidExtendedType, hrStatus, pvValue);
}