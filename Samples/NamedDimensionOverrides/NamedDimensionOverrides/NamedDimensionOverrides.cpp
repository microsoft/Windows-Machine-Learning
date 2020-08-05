// NamedDimensionOverrides.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include "pch.h"

using namespace winrt;
using namespace Microsoft::AI::MachineLearning;
using namespace Windows::Media;
using namespace Windows::Graphics::Imaging;
using namespace std;

int main()
{
    std::wstring modulePath = FileHelper::GetModulePath().c_str();
    std::wstring modelPath = modulePath + L"candy.onnx";
    
    // load model with variadic inputs
    printf("Loading modelfile '%ws' on the CPU\n", modelPath.c_str());
    auto model = LearningModel::LoadFromFilePath(modelPath);
    
    LearningModelDevice device(LearningModelDeviceKind::Default);

    std::vector<hstring> imageNames = { L"fish.png", L"kitten_224.png", L"fish.png" };

    // use session options to override to the exact dimension that will be input to the model
    LearningModelSessionOptions options;
    options.OverrideNamedDimension(L"None", static_cast<uint32_t>(imageNames.size()));

    LearningModelSession session = LearningModelSession(model, device, options);

    printf("Binding...\n");
    // Load input that match the overriden dimension
    std::vector<VideoFrame> inputFrames = {};
    for (hstring imageName : imageNames) {
        auto imagePath = static_cast<hstring>(FileHelper::GetModulePath().c_str()) + imageName;
        auto imageFrame = FileHelper::LoadImageFile(imagePath);
        inputFrames.emplace_back(imageFrame);
    }

    // bind the inputs to the session
    LearningModelBinding binding(session);
    auto inputFeatureDescriptor = model.InputFeatures().First();
    binding.Bind(inputFeatureDescriptor.Current().Name(), winrt::single_threaded_vector(std::move(inputFrames)));

    // create the output frames
    std::vector<VideoFrame> outputFrames = {};
    for (int i = 0; i < imageNames.size(); i++) {
        VideoFrame outputFrame(BitmapPixelFormat::Bgra8, 720, 720);
        outputFrames.emplace_back(outputFrame);
    }

    // bind the outputs to the session
    binding.Bind(L"outputImage", winrt::single_threaded_vector(std::move(outputFrames)));

    // run the model
    printf("Running the model...\n");
    DWORD ticks = GetTickCount();
    auto results = session.EvaluateAsync(binding, L"RunId").get();
    ticks = GetTickCount() - ticks;
    printf("model run took %d ticks\n", ticks);
}

