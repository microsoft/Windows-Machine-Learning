#include "TransformAsync.h"
#include "Helpers/CAutoLock.h"
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
            AutoLock lock(m_critSec);


            if (m_bShutdown == FALSE)
            {
                hr = MF_E_INVALIDREQUEST;
                break;
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
        AutoLock lock(m_critSec);


        hr = ShutdownEventQueue();
        if (FAILED(hr))
        {
            break;
        }

        /*hr = MFUnlockWorkQueue(m_dwDecodeWorkQueueID);
        if (FAILED(hr))
        {
            break;
        }*/

        m_bShutdown = TRUE;
    } while (false);

    return hr;
}
