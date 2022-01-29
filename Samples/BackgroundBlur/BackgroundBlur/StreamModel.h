#pragma once
#include <winrt/Microsoft.AI.MachineLearning.Experimental.h>
#include <winrt/Microsoft.AI.MachineLearning.h>
#include <Windows.AI.MachineLearning.native.h>
#include <winrt/Windows.Graphics.DirectX.Direct3D11.h>
#include <windows.media.core.interop.h>
#include <strsafe.h>
#include <wtypes.h>
#include <winrt/base.h>
#include <dxgi.h>
#include <d3d11.h>
#include <mutex>
#include <winrt/windows.foundation.collections.h>
#include <winrt/Windows.Media.h>
#include "TransformAsync.h"

using namespace winrt::Microsoft::AI::MachineLearning;
using namespace winrt::Microsoft::AI::MachineLearning::Experimental;
using namespace winrt::Windows::Graphics::DirectX::Direct3D11;
using namespace winrt::Windows::Media;

class StreamModel : public IMFAsyncCallback {
public: 
    // TODO: Need to return a new set of bindings each time
    // So that can run in parallel? 
    // Does this need a lock on the session then? (I think so) 
    HRESULT BindInput(const DWORD         dwDecodeWorkQueueID,
        IDirect3DSurface pInputSurface,
        __out IMFAsyncCallback** ppTask);

    // Sets up the IMFAsyncResult to use in Invoke
    HRESULT Begin(TransformAsync* pTransformAsync);

    // IMFAsyncCallback Implementations
    HRESULT __stdcall   GetParameters(
        DWORD* pdwFlags,
        DWORD* pdwQueue
    );
    HRESULT __stdcall   Invoke(
        IMFAsyncResult* pAsyncResult
    );

};