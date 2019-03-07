// main.cpp : Defines the entry point for the console application.
//

#include "pch.h"
#include "BatchSampleHelper.h"
#include <chrono>
#include <iostream>

#define N_RUN 20
#define BATCH_SIZE 20

using namespace winrt;
using namespace Windows::Foundation;
using namespace Windows::AI::MachineLearning;
using namespace Windows::Foundation::Collections;
using namespace Windows::Graphics::Imaging;
using namespace Windows::Media;
using namespace Windows::Storage;

using namespace std;
using namespace std::chrono;

EXTERN_C IMAGE_DOS_HEADER __ImageBase;

// Global variables
LearningModel model = nullptr;
LearningModelSession session = nullptr;
LearningModelBinding binding = nullptr;

const wstring modulePath{ BatchSampleHelper::GetModulePath() };

void LoadModel(hstring modelPath);

VideoFrame LoadImageFile(hstring filePath); 

IVector<VideoFrame> PrepareInputData(VideoFrame &singleVideoFrame, uint32_t batchSize);

std::vector<float> RunWithBatch(LearningModelDevice& device, VideoFrame &singleVideoFrame, uint32_t batchSize);

std::vector<float> RunWithoutBatch(LearningModelDevice& device, VideoFrame &singleVideoFrame);

int main()
{
    init_apartment();
    const hstring modelPath = static_cast<hstring>(modulePath.c_str()) + L"fns-candy.onnx";
    const hstring imagePath = static_cast<hstring>(modulePath.c_str()) + L"fish_720.png";

    LoadModel(modelPath);
    VideoFrame imageFrame = LoadImageFile(imagePath);

    printf("Iterations to Run : %d\n", N_RUN);
    printf("Batch Size is : %d\n", BATCH_SIZE);
    LearningModelDevice device = LearningModelDevice(LearningModelDeviceKind::DirectX);
    auto timeRecorderGPUBatch = RunWithBatch(device, imageFrame, BATCH_SIZE);
    auto timeRecorderGPUNonBatch = RunWithoutBatch(device, imageFrame);

    device = LearningModelDevice(LearningModelDeviceKind::Cpu);
    auto timeRecorderCPUBatch = RunWithBatch(device, imageFrame, BATCH_SIZE);
    auto timeRecorderCPUNonBatch = RunWithoutBatch(device, imageFrame);

    printf("%s", "Result on GPU (ms):\n");
    printf("%10s %10s %10s %10s %10s\n", " ", "FirstBind", "FirstEval", "AveBind", "AveEval");
    printf("%10s %10.2lf %10.2lf %10.2lf %10.2lf\n", "Batch", timeRecorderGPUBatch[0], timeRecorderGPUBatch[1], timeRecorderGPUBatch[2]/N_RUN, timeRecorderGPUBatch[3]/N_RUN);
    printf("%10s %10.2lf %10.2lf %10.2lf %10.2lf\n\n", "NonBatch", timeRecorderGPUNonBatch[0], timeRecorderGPUNonBatch[1], timeRecorderGPUNonBatch[2]/N_RUN, timeRecorderGPUNonBatch[3]/N_RUN);

    printf("%s", "Result on CPU (ms):\n");
    printf("%10s %10s %10s %10s %10s\n", " ", "FirstBind", "FirstEval", "AveBind", "AveEval");
    printf("%10s %10.2lf %10.2lf %10.2lf %10.2lf\n", "Batch", timeRecorderCPUBatch[0], timeRecorderCPUBatch[1], timeRecorderCPUBatch[2]/N_RUN, timeRecorderCPUBatch[3]/N_RUN);
    printf("%10s %10.2lf %10.2lf %10.2lf %10.2lf\n", "NonBatch", timeRecorderCPUNonBatch[0], timeRecorderCPUNonBatch[1], timeRecorderCPUNonBatch[2]/N_RUN, timeRecorderCPUNonBatch[3]/N_RUN);

}

void LoadModel(hstring modelPath)
{
    // load the model
    printf("Loading modelfile '%ws'\n", modelPath.c_str());
    try
    {
        model = LearningModel::LoadFromFilePath(modelPath);
    }
    catch (...)
    {
        printf("failed to load model, make sure you gave the right path\r\n");
        exit(EXIT_FAILURE);
    }
}

VideoFrame LoadImageFile(hstring filePath)
{
    VideoFrame inputImage = nullptr;

    try
    {
        // open the file
        StorageFile file = StorageFile::GetFileFromPathAsync(filePath).get();
        // get a stream on it
        auto stream = file.OpenAsync(FileAccessMode::Read).get();
        // Create the decoder from the stream
        BitmapDecoder decoder = BitmapDecoder::CreateAsync(stream).get();
        // get the bitmap
        SoftwareBitmap softwareBitmap = decoder.GetSoftwareBitmapAsync().get();
        // load a videoframe from it
        inputImage = VideoFrame::CreateWithSoftwareBitmap(softwareBitmap);
    }
    catch (...)
    {
        printf("failed to load the image file, make sure you are using fully qualified paths\r\n");
        exit(EXIT_FAILURE);
    }
    // all done
    return inputImage;
}

IVector<VideoFrame> PrepareInputData(VideoFrame &singleVideoFrame, uint32_t batchSize)
{
    std::vector<VideoFrame> inputFrames = {};
    for (uint32_t i = 0; i < batchSize; ++i)
    {
       inputFrames.emplace_back(singleVideoFrame);
    }
    return winrt::single_threaded_vector(std::move(inputFrames));
}

std::vector<float> RunWithBatch(LearningModelDevice& device, VideoFrame &singleVideoFrame, uint32_t batchSize)
{
    std::vector<float> timeRecorder; // in order of {FirstBindTime, FirstEvalTime, TotalBindTime, TotalEvalTime}, in microseconds
    printf("Running With Batch:\n");
    LearningModelSessionOptions options;
    options.BatchSizeOverride(batchSize);
    auto videoFrames = PrepareInputData(singleVideoFrame, batchSize);

    // create session and binding
    session = LearningModelSession(model, device, options);
    binding = LearningModelBinding{ session };

    // First Iteration:
    auto start = high_resolution_clock::now();
    binding.Bind(model.InputFeatures().First().Current().Name(), videoFrames);
    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(stop - start);
    timeRecorder.push_back(static_cast<float>(duration.count()) / 1000);

    start = high_resolution_clock::now();
    auto results = session.EvaluateAsync(binding, L"").get();
    stop = high_resolution_clock::now();
    duration = duration_cast<microseconds>(stop - start);
    timeRecorder.push_back(static_cast<float>(duration.count()) / 1000);

    // 2 - N_RUN Iterations:
    microseconds totalBindTime{ 0 };
    microseconds totalEvalTime{ 0 };
    for (uint32_t i = 0; i < N_RUN; ++i)
    {
        start = high_resolution_clock::now();
        binding.Bind(model.InputFeatures().First().Current().Name(), videoFrames);
        stop = high_resolution_clock::now();
        totalBindTime += duration_cast<microseconds>(stop - start);

        start = high_resolution_clock::now();
        results = session.EvaluateAsync(binding, L"").get();
        stop = high_resolution_clock::now();
        totalEvalTime += duration_cast<microseconds>(stop - start);
    }
    timeRecorder.push_back(static_cast<float>(totalBindTime.count()) / 1000);
    timeRecorder.push_back(static_cast<float>(totalEvalTime.count()) / 1000);
    return timeRecorder;
}

std::vector<float> RunWithoutBatch(LearningModelDevice& device, VideoFrame &singleVideoFrame)
{
    printf("Running Without Batch:\n");
    session = LearningModelSession(model, device);
    binding = LearningModelBinding{ session };

    std::vector<float> timeRecorder; // in order of {FirstBindTime, FirstEvalTime, TotalBindTime, TotalEvalTime}, in microseconds

    // First Iteration:
    auto start = high_resolution_clock::now();
    binding.Bind(model.InputFeatures().First().Current().Name(), singleVideoFrame);
    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(stop - start);
    timeRecorder.push_back(static_cast<float>(duration.count()) / 1000);

    start = high_resolution_clock::now();
    auto results = session.EvaluateAsync(binding, L"").get();
    stop = high_resolution_clock::now();
    duration = duration_cast<microseconds>(stop - start);
    timeRecorder.push_back(static_cast<float>(duration.count()) / 1000);

    // 2 - N_RUN Iterations:
    microseconds totalBindTime{ 0 };
    microseconds totalEvalTime{ 0 };
    for (uint32_t i = 0; i < N_RUN; ++i)
    {
        start = high_resolution_clock::now();
        binding.Bind(model.InputFeatures().First().Current().Name(), singleVideoFrame);
        stop = high_resolution_clock::now();
        totalBindTime += duration_cast<microseconds>(stop - start);

        start = high_resolution_clock::now();
        results = session.EvaluateAsync(binding, L"").get();
        stop = high_resolution_clock::now();
        totalEvalTime += duration_cast<microseconds>(stop - start);
    }
    timeRecorder.push_back(static_cast<float>(totalBindTime.count()) / 1000);
    timeRecorder.push_back(static_cast<float>(totalEvalTime.count()) / 1000);
    return timeRecorder;

}
