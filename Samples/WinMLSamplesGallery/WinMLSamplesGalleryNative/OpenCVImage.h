#pragma once
#include "OpenCVImage.g.h"

#ifdef USE_OPENCV
#include <opencv2/opencv.hpp>
#endif


namespace winrt::WinMLSamplesGalleryNative::implementation
{
    struct OpenCVImage : OpenCVImageT<OpenCVImage>
    {
        OpenCVImage(winrt::hstring path);
#ifdef USE_OPENCV
        OpenCVImage(cv::Mat&& image);
#endif

        static winrt::WinMLSamplesGalleryNative::OpenCVImage CreateFromPath(hstring const& path);
        static winrt::WinMLSamplesGalleryNative::OpenCVImage AddSaltAndPepperNoise(winrt::WinMLSamplesGalleryNative::OpenCVImage image);
        static winrt::WinMLSamplesGalleryNative::OpenCVImage DenoiseMedianBlur(winrt::WinMLSamplesGalleryNative::OpenCVImage image);
        winrt::Microsoft::AI::MachineLearning::ITensor AsTensor();
        winrt::Windows::Graphics::Imaging::SoftwareBitmap AsSoftwareBitmap();

        winrt::Windows::Storage::Streams::IBuffer AsWeakBuffer();
        void Close();

    private:
#ifdef USE_OPENCV
        cv::Mat image_;
#endif
    };
}
namespace winrt::WinMLSamplesGalleryNative::factory_implementation
{
    struct OpenCVImage : OpenCVImageT<OpenCVImage, implementation::OpenCVImage>
    {
    };
}
