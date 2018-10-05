#pragma once

#include <unknwn.h>
#include <winrt/Windows.Media.h>
#include <winrt/Windows.Graphics.Imaging.h>

namespace TensorizationHelper
{
    std::wstring GetModulePath();

    winrt::Windows::AI::MachineLearning::TensorFloat SoftwareBitmapToSoftwareTensor(
        winrt::Windows::Graphics::Imaging::SoftwareBitmap softwareBitmap);

    winrt::Windows::AI::MachineLearning::TensorFloat SoftwareBitmapToDX12Tensor(
        winrt::Windows::Graphics::Imaging::SoftwareBitmap softwareBitmap);
}