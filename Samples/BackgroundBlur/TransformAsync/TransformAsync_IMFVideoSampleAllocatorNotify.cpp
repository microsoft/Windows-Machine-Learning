#include "pch.h"
#include "TransformAsync.h"
#include <CommCtrl.h>
long long g_now; // The time since the last call to FrameThreadProc

DWORD __stdcall FrameThreadProc(LPVOID lpParam)
{
    DWORD waitResult;
    // Get the handle from the lpParam pointer
    //HANDLE event = lpParam;
    com_ptr<IUnknown> unk;
    com_ptr<TransformAsync> transform;
    unk.copy_from((IUnknown*)lpParam);
    transform = unk.as<TransformAsync>();

    //OutputDebugString(L"Thread %d waiting for Frame event...");
    waitResult = WaitForSingleObject(
        transform->m_fenceEvent.get(),         // event handle
        INFINITE);      // indefinite wait


    switch (waitResult) {
    case WAIT_OBJECT_0:
        // TODO: Capture time and write to preview
        if (g_now == NULL) {
            g_now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
            //OutputDebugString(L"First time responding to event!");
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
            transform->WriteFrameRate(message.c_str());
            //MainWindow::_SetStatusText(message.c_str());
            //TRACE(("Responded to event and it's been %d miliseconds", timePassed));
            // TODO: Call Set status text with new framerate
        }

        break;
    default:
        TRACE(("Wait error (%d)\n", GetLastError()));
        return 0;
    }
    return 1;
}

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
        m_frameThread.reset(CreateThread(NULL, 0, FrameThreadProc, this, 0, &dwThreadID));
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