#include "pch.h"
#include "OpenCVImage.h"
#include "OpenCVImage.g.cpp"

#include "winrt/Windows.Graphics.Imaging.h"
#include "winrt/Windows.Storage.Streams.h"
#include "winrt/Microsoft.AI.MachineLearning.h"

#include "WeakBuffer.h"
#include <wrl.h>
#include <sstream>

namespace wrl = ::Microsoft::WRL;
namespace details = ::Microsoft::AI::MachineLearning::Details;
namespace abi_wss = ABI::Windows::Storage::Streams;

namespace winrt::WinMLSamplesGalleryNative::implementation
{
    OpenCVImage::OpenCVImage(winrt::hstring path)
    {
#ifdef USE_OPENCV
        image_ = cv::imread(winrt::to_string(path), cv::IMREAD_COLOR);
#endif
    }

#ifdef USE_OPENCV
    OpenCVImage::OpenCVImage(cv::Mat&& image) : image_(std::move(image)) {  
    }
#endif

    winrt::Windows::Storage::Streams::IBuffer OpenCVImage::AsWeakBuffer()
    {
#ifdef USE_OPENCV
        auto cz_buffer = image_.ptr();
        auto size = image_.total()* image_.elemSize();
        winrt::com_ptr<abi_wss::IBuffer> ptr;
        wrl::MakeAndInitialize<details::WeakBuffer<uint8_t>>(ptr.put(), cz_buffer, cz_buffer + size);
        return ptr.as<winrt::Windows::Storage::Streams::IBuffer>();
#else
        return nullptr;
#endif
    }

    winrt::Microsoft::AI::MachineLearning::ITensor OpenCVImage::AsTensor()
    {
#ifdef USE_OPENCV
        auto buffer = AsWeakBuffer();
        return winrt::Microsoft::AI::MachineLearning::TensorUInt8Bit::CreateFromBuffer(
            std::vector<int64_t>{ 1, image_.rows, image_.cols, 3 }, buffer);
#else
        return nullptr;
#endif
    }


    winrt::Windows::Graphics::Imaging::SoftwareBitmap OpenCVImage::AsSoftwareBitmap()
    {
#ifdef USE_OPENCV
        cv::Mat bgra_image;
        cv::cvtColor(image_, bgra_image, cv::COLOR_RGB2RGBA);

        auto bgra_opencv_image = winrt::make<OpenCVImage>(std::move(bgra_image));
        auto bgra_opencv_image_impl = bgra_opencv_image.as<implementation::OpenCVImage>();
        auto bgra_buffer = bgra_opencv_image_impl->AsWeakBuffer();
        auto software_bitmap =
            winrt::Windows::Graphics::Imaging::SoftwareBitmap::CreateCopyFromBuffer(
                bgra_buffer, winrt::Windows::Graphics::Imaging::BitmapPixelFormat::Bgra8, image_.cols, image_.rows);
        return software_bitmap;
#else
        return nullptr;
#endif
    }

    void OpenCVImage::Close()
    {
#ifdef USE_OPENCV
        image_.deallocate();
#endif
    }

    winrt::WinMLSamplesGalleryNative::OpenCVImage OpenCVImage::CreateFromPath(hstring const& path) {
        return winrt::make<OpenCVImage>(path);
    }

    winrt::WinMLSamplesGalleryNative::OpenCVImage OpenCVImage::AddSaltAndPepperNoise(winrt::WinMLSamplesGalleryNative::OpenCVImage image) {
#ifdef USE_OPENCV
        auto& image_mat = image.as<implementation::OpenCVImage>().get()->image_;
        cv::Mat saltpepper_noise = cv::Mat::zeros(image_mat.rows, image_mat.cols, CV_8U);
        randu(saltpepper_noise, 0, 255);

        cv::Mat black = saltpepper_noise < 30;
        cv::Mat white = saltpepper_noise > 225;

        cv::Mat saltpepper_img = image_mat.clone();
        saltpepper_img.setTo(255, white);
        saltpepper_img.setTo(0, black);

        return winrt::make<OpenCVImage>(std::move(saltpepper_img));
#else
        return nullptr;
#endif
    }

    winrt::WinMLSamplesGalleryNative::OpenCVImage OpenCVImage::DenoiseMedianBlur(winrt::WinMLSamplesGalleryNative::OpenCVImage image) {
#ifdef USE_OPENCV
        auto& image_mat = image.as<implementation::OpenCVImage>().get()->image_;
        cv::Mat denoised;
        cv::medianBlur(image_mat, denoised, 5);
        return winrt::make<OpenCVImage>(std::move(denoised));
#else
        return nullptr;
#endif
    }
}
