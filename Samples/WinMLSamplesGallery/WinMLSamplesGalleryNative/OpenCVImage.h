#pragma once
#include "OpenCVImage.g.h"

#include <opencv2/opencv.hpp>

namespace winrt::WinMLSamplesGalleryNative::implementation
{
    struct OpenCVImage : OpenCVImageT<OpenCVImage>
    {
        OpenCVImage(winrt::hstring path);
        OpenCVImage(cv::Mat&& image);

        static winrt::WinMLSamplesGalleryNative::OpenCVImage CreateFromPath(hstring const& path);
        static winrt::WinMLSamplesGalleryNative::OpenCVImage AddSaltAndPepperNoise(winrt::WinMLSamplesGalleryNative::OpenCVImage image);
        static winrt::WinMLSamplesGalleryNative::OpenCVImage DenoiseMedianBlur(winrt::WinMLSamplesGalleryNative::OpenCVImage image);
        winrt::Microsoft::AI::MachineLearning::ITensor AsTensor();
        winrt::Windows::Graphics::Imaging::SoftwareBitmap AsSoftwareBitmap();

        winrt::Windows::Storage::Streams::IBuffer AsWeakBuffer();
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
