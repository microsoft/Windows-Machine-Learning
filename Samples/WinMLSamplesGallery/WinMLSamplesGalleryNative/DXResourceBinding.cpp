#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#include "pch.h"
#include "DXResourceBinding.h"
#include "DXResourceBinding.g.cpp"
#include <DirectXMath.h>
#include "stdafx.h"
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
#include "ORTHelpers.h"

#include "Win32Application.h"

#undef min

Microsoft::WRL::ComPtr<ID3D12Resource> d3dResource;
static std::optional<Ort::Session> preprocesingSession;
static std::optional<Ort::Session> inferenceSession;
D3D12Quad sample(800, 600, L"D3D12 Quad");

namespace winrt::WinMLSamplesGalleryNative::implementation
{
	winrt::com_array<float> DXResourceBinding::LaunchWindow() {

        const wchar_t* preprocessingModelFilePath = L"C:/Users/numform/Windows-Machine-Learning/Samples/WinMLSamplesGallery/WinMLSamplesGalleryNative/dx_preprocessor_efficient_net.onnx";
        preprocesingSession = CreateSession(preprocessingModelFilePath);

        const wchar_t* inferencemodelFilePath = L"C:/Users/numform/Windows-Machine-Learning/Samples/WinMLSamplesGallery/WinMLSamplesGalleryNative/efficientnet-lite4-11.onnx";
        inferenceSession = CreateSession(inferencemodelFilePath);

        std::thread d3d_th(Win32Application::Run, &sample, 10);

        // Wait until the D3D pipeline finishes
        while (!sample.is_initialized) {}

        // Run the quad in a separate, detached thread
        d3d_th.detach();

        winrt::com_array<float> eval_results(1000);
        for (int i = 0; i < 1000; i++) {
            eval_results[i] = 100;
        }
        return eval_results;
	}

    winrt::com_array<float> DXResourceBinding::EvalORT() {
        D3D12Quad::D3DInfo info = sample.GetD3DInfo();
        bool running = true;
        return Preprocess(*preprocesingSession, *inferenceSession, info.device.Get(),
            running, info.swapChain.Get(), info.frameIndex, info.commandAllocator.Get(), info.commandList.Get(),
            info.commandQueue.Get());
    }

    void DXResourceBinding::CloseWindow() {
        Win32Application::CloseWindow();
    }
}