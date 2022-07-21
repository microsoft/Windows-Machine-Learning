#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#include "pch.h"
#include "DXResourceBinding.h"
#include "DXResourceBinding.g.cpp"
#include "stdafx.h"
#include "ORTHelpers.h"
#include "winrt/Microsoft.AI.MachineLearning.h"
#include "winrt/Microsoft.AI.MachineLearning.Experimental.h"

using namespace winrt::Microsoft::AI::MachineLearning;

#undef min

Microsoft::WRL::ComPtr<ID3D12Resource> d3dResource;
static std::optional<Ort::Session> preprocesingSession;
static std::optional<Ort::Session> inferenceSession;
D3D12Quad sample(800, 600, L"D3D12 Quad");

using TensorInt64Bit = winrt::Microsoft::AI::MachineLearning::TensorInt64Bit;
using TensorKind = winrt::Microsoft::AI::MachineLearning::TensorKind;
using LearningModelBuilder = winrt::Microsoft::AI::MachineLearning::Experimental::LearningModelBuilder;
using LearningModelOperator = winrt::Microsoft::AI::MachineLearning::Experimental::LearningModelOperator;

std::array<int64_t, 2> preprocessInputShape;

std::array<long, 6> CalculateCenterFillDimensions(long oldH, long oldW, long h, long w)
{
    long resizedW, resizedH, top, bottom, left, right;
    auto oldHFloat = (float)oldH;
    auto oldWFloat = (float)oldW;
    auto hFloat = (float)h;
    auto wFloat = (float)w;

    auto oldAspectRatio = oldWFloat / oldHFloat;
    auto newAspectRatio = wFloat / hFloat;

    auto scale = (newAspectRatio < oldAspectRatio) ? (hFloat / oldHFloat) : (wFloat / oldWFloat);
    resizedW = (newAspectRatio < oldAspectRatio) ? (long)std::floor(scale * oldWFloat) : w;
    resizedH = (newAspectRatio < oldAspectRatio) ? h : (long)std::floor(scale * oldHFloat);
    long totalPad = (newAspectRatio < oldAspectRatio) ? resizedW - w : resizedH - h;
    long biggerDim = (newAspectRatio < oldAspectRatio) ? w : h;
    long first = (totalPad % 2 == 0) ? totalPad / 2 : (long)std::floor(totalPad / 2.0f);
    long second = first + biggerDim;

    if (newAspectRatio < oldAspectRatio)
    {
        top = 0;
        bottom = h;
        left = first;
        right = second;
    }
    else
    {
        top = first;
        bottom = second;
        left = 0;
        right = w;
    }

    std::array<long, 6> new_dimensions = { resizedW, resizedH, top, bottom, left, right };
    return new_dimensions;
}

namespace winrt::WinMLSamplesGalleryNative::implementation
{
    // Create ORT Sessions and launch D3D window in a separate thread
	void DXResourceBinding::LaunchWindow() {


        long newH = 224;
        long newW = 224;
        long h = 512;
        long w = 512;
        std::array<long, 6> center_fill_dimensions = CalculateCenterFillDimensions(h, w, newH, newW);
        long resizedW = center_fill_dimensions[0];
        long resizedH = center_fill_dimensions[1];
        long top = center_fill_dimensions[2];
        long bottom = center_fill_dimensions[3];
        long left = center_fill_dimensions[4];
        long right = center_fill_dimensions[5];
        winrt::hstring interpolationMode = L"nearest";
        long c = 4;

        auto width = 800;
        auto height = 600;
        auto rowPitchInBytes = (width * 4 + 255) & ~255;
        auto rowPitchInPixels = rowPitchInBytes / 4;
        auto bufferInBytes = rowPitchInBytes * height;
        preprocessInputShape = { 1, bufferInBytes};
        //const std::array<int64_t, 4> preprocessInputShape = { 1, 512, 512, 4 };
        //const std::array<int64_t, 4> preprocessInputShape = { 1, -1, -1, -1 };

        const std::array<int64_t, 4> preprocessOutputShape = { 1, 224, 224, 3 };

        auto kernel = new float[] {
            0,0,1,
            0,1,0,
            1,0,0
        };

        // Create ORT Sessions that will be used for preprocessing and classification
        preprocesingSession = CreateSession(Win32Application::GetAssetPath(L"dx_preprocessor_efficient_net_v2.onnx").c_str());
        inferenceSession = CreateSession(Win32Application::GetAssetPath(L"efficientnet-lite4-11.onnx").c_str());

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
            currentBuffer, preprocessInputShape);

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