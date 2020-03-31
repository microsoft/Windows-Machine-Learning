#pragma once
#include <string>
#include <winrt/Windows.Media.h>
#include <Windows.h>

namespace FileHelper
{
  // Get the Path of Executable
  std::wstring GetModulePath();

  // Load Image File as VideoFrame
  winrt::Windows::Media::VideoFrame LoadImageFile(
    winrt::hstring filePath);

  // Load object detection labels
  std::vector<std::string> LoadLabels(std::string labelsFilePath);
}
