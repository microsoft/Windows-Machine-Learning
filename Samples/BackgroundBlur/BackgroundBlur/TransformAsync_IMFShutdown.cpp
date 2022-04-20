#include "TransformAsync.h"
#include <mferror.h>
#include <mfapi.h>

HRESULT TransformAsync::GetShutdownStatus(
    MFSHUTDOWN_STATUS* pStatus)
{

    if (pStatus == NULL)
    {
        return E_POINTER;
    }

    {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);


        if (m_bShutdown == FALSE)
        {
            return MF_E_INVALIDREQUEST;
        }

        if (m_spOutputSampleAllocator) {
            m_spOutputSampleAllocator->UninitializeSampleAllocator();
        }

        *pStatus = MFSHUTDOWN_COMPLETED;
    }

    return S_OK;
}

HRESULT TransformAsync::Shutdown(void)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    RETURN_IF_FAILED(ShutdownEventQueue());
    m_bShutdown = TRUE;
    return S_OK;
}
