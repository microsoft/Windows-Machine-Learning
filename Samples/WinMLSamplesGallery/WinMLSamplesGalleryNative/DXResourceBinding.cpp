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
    // Create ORT Sessions and launch D3D window in a separate thread
	void DXResourceBinding::LaunchWindow() {
        // Create ORT Sessions that will be used for preprocessing and classification
        const wchar_t* preprocessingModelFilePath = L"C:/Users/numform/Windows-Machine-Learning/Samples/WinMLSamplesGallery/WinMLSamplesGalleryNative/dx_preprocessor_efficient_net.onnx";
        const wchar_t* inferencemodelFilePath = L"C:/Users/numform/Windows-Machine-Learning/Samples/WinMLSamplesGallery/WinMLSamplesGalleryNative/efficientnet-lite4-11.onnx";
        preprocesingSession = CreateSession(preprocessingModelFilePath);
        inferenceSession = CreateSession(inferencemodelFilePath);

        // Spawn the window in a separate thread
        std::thread d3d_th(Win32Application::Run, &sample, 10);

        // Wait until the D3D pipeline finishes
        while (!sample.is_initialized) {}

        // Detach the thread so it doesn't block the UI
        d3d_th.detach();
	}

    // Get the buffer currently being drawn to the screen then
    // preprocess and classify it
    winrt::com_array<float> DXResourceBinding::EvalORT() {
        // Get the buffer currently being drawn to the screen
        ComPtr<ID3D12Resource> currentBuffer = sample.GetCurrentBuffer();

        // Preprocess the buffer (shrink from 512 x 512 x 4 to 224 x 224 x 3)
        Ort::Value preprocessedInput = Preprocess(*preprocesingSession,
            currentBuffer);

        // Classify the image using EfficientNet and return the results
        winrt::com_array<float> results = Eval(*inferenceSession, preprocessedInput);

        return results;
    }

    // Close the D3D Window and cleanup
    void DXResourceBinding::CloseWindow() {
        Win32Application::CloseWindow();
    }
}