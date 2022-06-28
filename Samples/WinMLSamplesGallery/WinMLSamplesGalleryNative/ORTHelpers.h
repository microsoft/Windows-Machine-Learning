#pragma once

#include "winrt/Microsoft.AI.MachineLearning.h"
#include "winrt/Microsoft.AI.MachineLearning.Experimental.h"

#include <DirectXMath.h>
#include <commctrl.h>
#include <mfapi.h>
#include <iostream>

#include <cstdio>
#include <algorithm>
#include <numeric>
#include <functional>
#include <utility>
#include <string_view>
#include <span>
#include <optional>
#include <memory>

#include <windows.h>
#include <d3d12.h>
#include <wrl/client.h>
#include "dml_provider_factory.h"
#include "onnxruntime_cxx_api.h"

#include <dxgi1_4.h>
#include <D3Dcompiler.h>
#include "d3dx12.h"
#include <string>
#include <wincodec.h>
#include <ScreenGrab.h>

using namespace winrt::Microsoft::AI::MachineLearning;
using namespace Microsoft::WRL;

Ort::Session CreateSession(const wchar_t* model_file_path);

std::vector<float> Eval(Ort::Session& session, const Ort::Value& prev_input);

winrt::com_array<float> Preprocess(Ort::Session& session,
    Ort::Session& inferenceSession,
    ID3D12Device* device,
    bool& Running,
    IDXGISwapChain3* swapChain,
    UINT frameIndex,
    ID3D12CommandAllocator* commandAllocator,
    ID3D12GraphicsCommandList* commandList,
    ID3D12CommandQueue* commandQueue);

Ort::Value CreateTensorValueUsingD3DResource(
    ID3D12Device* d3d12Device,
    OrtDmlApi const& ortDmlApi,
    Ort::MemoryInfo const& memoryInformation,
    std::span<const int64_t> tensorDimensions,
    ONNXTensorElementDataType elementDataType,
    size_t elementByteSize,
    /*out*/ void** dmlEpResourceWrapper // Must stay alive with Ort::Value.
);

Ort::Value CreateTensorValueFromRTVResource(
    OrtDmlApi const& ortDmlApi,
    Ort::MemoryInfo const& memoryInformation,
    ID3D12Resource* d3dResource,
    std::span<const int64_t> tensorDimensions,
    ONNXTensorElementDataType elementDataType,
    /*out*/ void** dmlEpResourceWrapper // Must stay alive with Ort::Value.
);

std::array<long, 6> CalculateCenterFillDimensions(long oldH, long oldW, long h, long w);