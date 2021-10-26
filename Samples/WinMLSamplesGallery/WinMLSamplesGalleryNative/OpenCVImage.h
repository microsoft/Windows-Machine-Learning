#pragma once
#include "OpenCVImage.g.h"

#include <opencv2/opencv.hpp>

namespace winrt::WinMLSamplesGalleryNative::implementation
{
    struct OpenCVImage : OpenCVImageT<OpenCVImage>
    {
        OpenCVImage(winrt::hstring path);

        static winrt::WinMLSamplesGalleryNative::OpenCVImage CreateFromPath(hstring const& path);
        winrt::Microsoft::AI::MachineLearning::ITensor AsTensor();
        void Close();

    private:
        cv::Mat image_;
    };
}
namespace winrt::WinMLSamplesGalleryNative::factory_implementation
{
    struct OpenCVImage : OpenCVImageT<OpenCVImage, implementation::OpenCVImage>
    {
    };
}
