#pragma once

#include <string>
#include <unknwn.h>
#include <winrt/Windows.Media.h>
#include <winrt/Windows.Graphics.Imaging.h>

namespace SampleHelper
{
  // Return the path where BatchSupport.exe is located
  std::wstring GetModulePath();

  // Convert SoftwareBitmap to std::vector<float>
  std::vector<float> SoftwareBitmapToSoftwareTensor(
    winrt::Windows::Graphics::Imaging::SoftwareBitmap softwareBitmap);

  // Load Image File as VideoFrame
  winrt::Windows::Media::VideoFrame LoadImageFile(
    winrt::hstring filePath);

  // Load object detection labels
  std::vector<std::string> LoadLabels(std::string labelsFilePath);

  // Create input Tensorfloats with 3 images.
  winrt::Windows::AI::MachineLearning::TensorFloat CreateInputTensorFloat();

  // Create input VideoFrames with 3 images
  winrt::Windows::Foundation::Collections::IVector<winrt::Windows::Media::VideoFrame> CreateVideoFrames();

  winrt::hstring GetModelPath(std::string modelType);

  void PrintResults(winrt::Windows::Foundation::Collections::IVectorView<float> results);

}