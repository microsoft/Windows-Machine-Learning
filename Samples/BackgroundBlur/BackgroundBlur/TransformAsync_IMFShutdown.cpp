#include "TransformAsync.h"
#include <mferror.h>
#include <mfapi.h>

HRESULT TransformAsync::GetShutdownStatus(
    MFSHUTDOWN_STATUS* pStatus)
{
    HRESULT hr = S_OK;

    do
    {
        if (pStatus == NULL)
        {
            hr = E_POINTER;
            break;
        }

        {
            std::lock_guard<std::recursive_mutex> lock(m_mutex);


            if (m_bShutdown == FALSE)
            {
                hr = MF_E_INVALIDREQUEST;
                break;
            }

            if (m_spOutputSampleAllocator) {
                m_spOutputSampleAllocator->UninitializeSampleAllocator();
            }

            *pStatus = MFSHUTDOWN_COMPLETED;
        }
    } while (false);

    return hr;
}

HRESULT TransformAsync::Shutdown(void)
{
    HRESULT hr = S_OK;

    do
    {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);


        hr = ShutdownEventQueue();
        if (FAILED(hr))
        {
            break;
        }

        m_bShutdown = TRUE;
    } while (false);

    return hr;
}
