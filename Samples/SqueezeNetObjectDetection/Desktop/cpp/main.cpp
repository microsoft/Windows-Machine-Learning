// main.cpp : Defines the entry point for the console application.
//

#include "pch.h"
#include "FileHelper.h"
#include <winrt/Microsoft.AI.MachineLearning.Experimental.h>
#include "MelSpectrogram.h"

using namespace winrt;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;

using namespace Microsoft::AI::MachineLearning;

using namespace Windows::Media;
using namespace Windows::Graphics::Imaging;
using namespace Windows::Storage;
using namespace std;

// globals
vector<string> labels;
string labelsFileName("labels.txt");
LearningModelDeviceKind deviceKind = LearningModelDeviceKind::Default;
string deviceName = "default";
hstring imagePath;

VideoFrame LoadImageFile(hstring filePath, ColorManagementMode colorManagementMode);
void PrintResults(IVectorView<float> results);
bool ParseArgs(int argc, char* argv[]);
ColorManagementMode GetColorManagementMode(const LearningModel& model);

wstring GetModelPath()
{
    wostringstream woss;
    woss << FileHelper::GetModulePath().c_str();
    woss << "SqueezeNet.onnx";
    return woss.str();
}

// MAIN !
// usage: SqueezeNet [imagefile] [cpu|directx]
int main(int argc, char* argv[])
{
    // did they pass in the args 
    if (ParseArgs(argc, argv) == false)
    {
        printf("Usage: %s soundfile [amplitude]", argv[0]);
        return -1;
    }

    printf("Melspec on 3 tone signal \n");

    size_t batch_size = 1;
    size_t sample_rate = 8192;
    float signal_duration_in_seconds = 5.f;
    size_t signal_size = static_cast<size_t>(sample_rate * signal_duration_in_seconds);
    size_t dft_size = 256;
    size_t hop_size = 128;
    size_t window_size = 256;
    size_t n_mel_bins = 1024;
    

    MelSpectrogram::MelSpectrogramOnThreeToneSignal(batch_size, signal_size, window_size, dft_size,
        hop_size, n_mel_bins, sample_rate);
    printf("Printed 3 tone signal melspectrogram");


    printf("Melspec on 1 second recording \n");

    batch_size = 1;
    sample_rate = 8192;
    signal_duration_in_seconds = 1.f;
     dft_size = 256;
     hop_size = 3;
     window_size = 256;
     n_mel_bins = 1024;
     size_t amplitude = atoi(argv[2]);

    MelSpectrogram::MelSpectrogramOnFile(argv[1], batch_size, window_size, dft_size,
        hop_size, n_mel_bins, sample_rate, amplitude);
    printf("Printed spectrogram for file selected");

    printf("done");
  
}



bool ParseArgs(int argc, char* argv[])
{
    if (argc < 2)
    {
        return false;
    }
    // get the image file
    imagePath = hstring(wstring_to_utf8().from_bytes(argv[1]));
    // did they pass a third arg?
    if (argc >= 3)
    {
        deviceName = argv[2];
        if (deviceName == "cpu")
        {
            deviceKind = LearningModelDeviceKind::Cpu;
        }
        else if (deviceName == "directx")
        {
            deviceKind = LearningModelDeviceKind::DirectX;
        }
        else
        {
            deviceName = "default";
            deviceKind = LearningModelDeviceKind::Default;
        }
    }
    return true;
}


//
//ColorManagementMode GetColorManagementMode(const LearningModel& model)
//{
//    // Get model color space gamma
//    hstring gammaSpace = L"";
//    try
//    {
//        gammaSpace = model.Metadata().Lookup(L"Image.ColorSpaceGamma");
//    }
//    catch (...)
//    {
//        printf("    Model does not have color space gamma information. Will color manage to sRGB by default...\n");
//    }
//    if (gammaSpace == L"" || _wcsicmp(gammaSpace.c_str(), L"SRGB") == 0)
//    {
//        return ColorManagementMode::ColorManageToSRgb;
//    }
//    // Due diligence should be done to make sure that the input image is within the model's colorspace. There are multiple non-sRGB color spaces.
//    printf("    Model metadata indicates that color gamma space is : %ws. Will not manage color space to sRGB...\n", gammaSpace.c_str());
//    return ColorManagementMode::DoNotColorManage;
//}
//
//VideoFrame LoadImageFile(hstring filePath, ColorManagementMode colorManagementMode)
//{
//    BitmapDecoder decoder = NULL;
//    try
//    {
//        // open the file
//        StorageFile file = StorageFile::GetFileFromPathAsync(filePath).get();
//        // get a stream on it
//        auto stream = file.OpenAsync(FileAccessMode::Read).get();
//        // Create the decoder from the stream
//        decoder = BitmapDecoder::CreateAsync(stream).get();
//    }
//    catch (...)
//    {
//        printf("    Failed to load the image file, make sure you are using fully qualified paths\r\n");
//        exit(EXIT_FAILURE);
//    }
//    SoftwareBitmap softwareBitmap = NULL;
//    try
//    {
//        softwareBitmap = decoder.GetSoftwareBitmapAsync(
//            decoder.BitmapPixelFormat(),
//            decoder.BitmapAlphaMode(),
//            BitmapTransform(),
//            ExifOrientationMode::RespectExifOrientation,
//            colorManagementMode
//        ).get();
//    }
//    catch (hresult_error hr)
//    {
//        printf("    Failed to create SoftwareBitmap! Please make sure that input image is within the model's colorspace.\n");
//        printf("    %ws\n", hr.message().c_str());
//        exit(hr.code());
//    }
//    VideoFrame inputImage = NULL;
//    try
//    {
//        // load a videoframe from it
//        inputImage = VideoFrame::CreateWithSoftwareBitmap(softwareBitmap);
//    }
//    catch (hresult_error hr)
//    {
//        printf("Failed to create videoframe from software bitmap.");
//        printf("    %ws\n", hr.message().c_str());
//        exit(hr.code());
//    }
//    // all done
//    return inputImage;
//}
//
//void PrintResults(IVectorView<float> results)
//{
//    // load the labels
//    auto modulePath = FileHelper::GetModulePath();
//    std::string labelsFilePath =
//      std::string(modulePath.begin(), modulePath.end()) + labelsFileName;
//    labels = FileHelper::LoadLabels(labelsFilePath);
//
//    vector<pair<float, uint32_t>> sortedResults;
//    for (uint32_t i = 0; i < results.Size(); i++) {
//        pair<float, uint32_t> curr;
//        curr.first = results.GetAt(i);
//        curr.second = i;
//        sortedResults.push_back(curr);
//    }
//    std::sort(sortedResults.begin(), sortedResults.end(),
//        [](pair<float, uint32_t> const &a, pair<float, uint32_t> const &b) { return a.first > b.first; });
//
//    // Display the result
//    for (int i = 0; i < 3; i++)
//    {
//        pair<float, uint32_t> curr = sortedResults.at(i);
//        printf("%s with confidence of %f\n", labels[curr.second].c_str(), curr.first);
//    }
//}