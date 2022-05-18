#include "pch.h"
#include "TransformAsync.h"
#include <CommCtrl.h>
long long g_now; // The time since the last call to FrameThreadProc

HRESULT TransformAsync::NotifyRelease()
{
    const UINT64 currFenceValue = m_fenceValue;
    auto fenceComplete = m_fence->GetCompletedValue();
    DWORD dwThreadID;

    // Fail fast if context doesn't exist anymore. 
    if (m_context == nullptr)
    {
        return S_OK;
    }

    // Scheduel a Signal command in the queue
    RETURN_IF_FAILED(m_context->Signal(m_fence.get(), currFenceValue));

    if (currFenceValue % FRAME_RATE_UPDATE == 0)
    {
        m_fence->SetEventOnCompletion(currFenceValue, m_fenceEvent.get()); // Raise FenceEvent when done

        DWORD waitResult = WaitForSingleObject(
            m_fenceEvent.get(),         // event handle
            300);                       // only wait 300 ms
        switch (waitResult) {
        case WAIT_OBJECT_0:
            if (g_now == NULL) {
                g_now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
            }
            else {
                auto l_now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
                auto timePassed = l_now - g_now;
                g_now = l_now;
                auto fps = 30000 / timePassed; // TODO: marco on num frames to update after? 
                OutputDebugString(L"THREAD: ");
                OutputDebugString(std::to_wstring(fps).c_str());
                OutputDebugString(L"\n");
                auto message = std::wstring(L"Frame Rate: ") + std::to_wstring(fps) + L" FPS";
                WriteFrameRate(message.c_str());
            }
            break;
        default:
            TRACE(("Wait error (%d)\n", GetLastError()));
        }
    }

    m_fenceValue = currFenceValue + 1;
    return S_OK;
}

void TransformAsync::SetFrameRateWnd(HWND hwnd)
{
    m_frameWnd = hwnd;
}

void TransformAsync::WriteFrameRate(const WCHAR* frameRate)
{
    if (m_frameWnd) {
        SendMessage(m_frameWnd, SB_SETTEXT, (WPARAM)(0), (LPARAM)frameRate);
    }
}