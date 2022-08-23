#include "pch.h"
#include "DXResourceBinding.h"
#include "DXResourceBinding.g.cpp"
#include "stdafx.h"
#include "ORTHelpers.h"

using namespace winrt::Microsoft::AI::MachineLearning;

Microsoft::WRL::ComPtr<ID3D12Resource> d3dResource;
static std::optional<Ort::Session> preprocesingSession;
static std::optional<Ort::Session> inferenceSession;
D3D12Quad sample(800, 600, L"D3D12 Quad");

namespace winrt::WinMLSamplesGalleryNative::implementation
{
    // Create ORT Sessions and launch D3D window in a separate thread
	void DXResourceBinding::LaunchWindow() {
        // Create ORT Sessions that will be used for preprocessing and classification
        preprocesingSession = CreateSession(Win32Application::GetAssetPath(L"dx_preprocessor_efficient_net.onnx").c_str());
        inferenceSession = CreateSession(Win32Application::GetAssetPath(L"efficientnet-lite4-11.onnx").c_str());

        // Spawn the window in a separate thread
        std::jthread d3d_th(Win32Application::Run, &sample, 10);

        // Wait until the D3D pipeline finishes
        sample.initializationSemaphore.acquire();

        // Detach the thread so it doesn't block the UI
        d3d_th.detach();
	}

    // Get the buffer currently being drawn to the screen then
    // preprocess and classify it
    winrt::com_array<float> DXResourceBinding::EvalORT() {
        // Get the buffer currently being drawn to the screen
        ComPtr<ID3D12Resource> currentBuffer = sample.GetCurrentBuffer();

        // Preprocess the buffer (shrink to 224 x 224 x 3)
        Ort::Value preprocessedInput = Preprocess(*preprocesingSession,
            currentBuffer);

        // Classify the image using EfficientNet and return the results
        winrt::com_array<float> results = Eval(*inferenceSession, preprocessedInput);

        sample.ShowNextImage();

        return results;
    }

    // Close the D3D Window and cleanup
    void DXResourceBinding::CloseWindow() {
        Win32Application::CloseWindow();
    }
}