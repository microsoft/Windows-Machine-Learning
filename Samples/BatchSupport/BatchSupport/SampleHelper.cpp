#include "pch.h"
#include "SampleHelper.h"
#include "Windows.AI.MachineLearning.Native.h"
#include <MemoryBuffer.h>
#include <windows.h>

#define CHECK_HRESULT winrt::check_hresult
using namespace winrt;
using namespace winrt::Windows::AI::MachineLearning;
using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Foundation::Collections;
using namespace winrt::Windows::Media;
using namespace winrt::Windows::Graphics::Imaging;
using namespace winrt::Windows::Storage::Streams;
using namespace winrt::Windows::Storage;

#define BATCH_SIZE 3

namespace SampleHelper {

std::vector<float>
SoftwareBitmapToFloatVector(SoftwareBitmap softwareBitmap) {
  /* Manully tensorize from CPU resource, steps:
  1. Get the access to buffer of softwarebitmap
  2. Transform the data in buffer to a vector of float
  */

  // 1. Get the access to buffer of softwarebitmap
  BYTE *pData = nullptr;
  UINT32 size = 0;
  BitmapBuffer spBitmapBuffer(
      softwareBitmap.LockBuffer(BitmapBufferAccessMode::Read));
  winrt::Windows::Foundation::IMemoryBufferReference reference =
      spBitmapBuffer.CreateReference();
  auto spByteAccess =
      reference.as<::Windows::Foundation::IMemoryBufferByteAccess>();
  CHECK_HRESULT(spByteAccess->GetBuffer(&pData, &size));

  uint32_t height = softwareBitmap.PixelHeight();
  uint32_t width = softwareBitmap.PixelWidth();
  BitmapPixelFormat pixelFormat = softwareBitmap.BitmapPixelFormat();
  uint32_t channels = BitmapPixelFormat::Gray8 == pixelFormat ? 1 : 3;

  std::vector<int64_t> shape = {1, channels, height, width};
  std::vector<float> outputVector(channels * height * width);

  // The channels of image stored in buffer is in order of BGRA-BGRA-BGRA-BGRA.
  // Then we transform it to the order of
  // BBBBB....GGGGG....RRRR....AAAA(dropped)

  // 2. Transform the data in buffer to a vector of float
  if (BitmapPixelFormat::Bgra8 == pixelFormat) {
    for (uint32_t i = 0; i < size; i += 4) {
      // suppose the model expects BGR image.
      // index 0 is B, 1 is G, 2 is R, 3 is alpha(dropped).
      uint32_t pixelInd = i / 4;
      outputVector[pixelInd] = (float)pData[i];
      outputVector[(height * width) + pixelInd] = (float)pData[i + 1];
      outputVector[(height * width * 2) + pixelInd] = (float)pData[i + 2];
    }
  }

  // Pixel Value Normalization can be done at here. We are using the range from
  // 0-255, but the range can be normilized to 0-1 before we return
  return outputVector;
}

hstring GetModelPath(std::string modelType) {
  hstring modelPath;
  if (modelType == "fixedBatchSize") {
    modelPath =
        static_cast<hstring>(FileHelper::GetModulePath().c_str()) + L"SqueezeNet_batch3.onnx";
  } else {
    modelPath =
        static_cast<hstring>(FileHelper::GetModulePath().c_str()) + L"SqueezeNet_free.onnx";
  }
  return modelPath;
}

TensorFloat CreateInputTensorFloat() {
  std::vector<hstring> imageNames = {L"fish.png", L"kitten_224.png", L"fish.png"};
  std::vector<float> inputVector = {};
  for (hstring imageName : imageNames) {
    auto imagePath = static_cast<hstring>(FileHelper::GetModulePath().c_str()) + imageName;
    auto imageFrame = FileHelper::LoadImageFile(imagePath);
    std::vector<float> imageVector =
      SoftwareBitmapToFloatVector(imageFrame.SoftwareBitmap());
    inputVector.insert(inputVector.end(), imageVector.begin(), imageVector.end());
  }

  // 224, 224 below are height and width specified in model input.
  auto inputShape = std::vector<int64_t>{ BATCH_SIZE, 3, 224, 224 };
  auto inputValue = TensorFloat::CreateFromIterable(
    inputShape,
    single_threaded_vector<float>(std::move(inputVector)).GetView());
 
  return inputValue;
}

IVector<VideoFrame> CreateVideoFrames() {
  std::vector<hstring> imageNames = { L"fish.png", L"kitten_224.png", L"fish.png" };
  std::vector<VideoFrame> inputFrames = {};
  for (hstring imageName : imageNames) {
    auto imagePath = static_cast<hstring>(FileHelper::GetModulePath().c_str()) + imageName;
    auto imageFrame = FileHelper::LoadImageFile(imagePath);
    inputFrames.emplace_back(imageFrame);
  }
  auto videoFrames = winrt::single_threaded_vector(std::move(inputFrames));
  return videoFrames;
}

void PrintResults(IVectorView<float> results) {
  // load the labels
  auto modulePath = FileHelper::GetModulePath();
  std::string labelsFilePath =
      std::string(modulePath.begin(), modulePath.end()) + "Labels.txt";
  std::vector<std::string> labels = FileHelper::LoadLabels(labelsFilePath);
  // SqueezeNet returns a list of 1000 options, with probabilities for each,
  // loop through all
  for (uint32_t batchId = 0; batchId < BATCH_SIZE; ++batchId) {
    // Find the top probability
    float topProbability = 0;
    int topProbabilityLabelIndex;
    uint32_t oneOutputSize = results.Size() / BATCH_SIZE;
    for (uint32_t i = 0; i < oneOutputSize; i++) {
      if (results.GetAt(i + oneOutputSize * batchId) > topProbability) {
        topProbabilityLabelIndex = i;
        topProbability = results.GetAt(i + oneOutputSize * batchId);
      }
    }
    // Display the result
    printf("Result for No.%d input \n", batchId);
    printf("%s with confidence of %f\n",
           labels[topProbabilityLabelIndex].c_str(), topProbability);
  }
}
} // namespace SampleHelper