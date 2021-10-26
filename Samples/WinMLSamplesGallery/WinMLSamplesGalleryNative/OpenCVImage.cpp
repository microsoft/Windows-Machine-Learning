#include "pch.h"
#include "OpenCVImage.h"
#include "OpenCVImage.g.cpp"

#include "winrt/Windows.Graphics.Imaging.h"
#include "winrt/Windows.Storage.Streams.h"
#include "winrt/Microsoft.AI.MachineLearning.h"

#include "WeakBuffer.h"

#include <wrl.h>

namespace wrl = ::Microsoft::WRL;
namespace details = ::Microsoft::AI::MachineLearning::Details;
namespace abi_wss = ABI::Windows::Storage::Streams;

namespace winrt::WinMLSamplesGalleryNative::implementation
{
    OpenCVImage::OpenCVImage(winrt::hstring path)
    {
        image_ = cv::imread(winrt::to_string(path), cv::IMREAD_COLOR);
    }


    winrt::Microsoft::AI::MachineLearning::ITensor OpenCVImage::AsTensor()
    {
        auto cz_buffer = image_.ptr();
        auto size = image_.total();
        winrt::com_ptr<abi_wss::IBuffer> ptr;
        wrl::MakeAndInitialize<details::WeakBuffer<uint8_t>>(ptr.put(), cz_buffer, cz_buffer + size);
        auto buffer = ptr.as<winrt::Windows::Storage::Streams::IBuffer>();
        return winrt::Microsoft::AI::MachineLearning::TensorUInt8Bit::CreateFromBuffer(
            std::vector<int64_t>{ 1, image_.rows, image_.cols, 3 }, buffer);
    }

    void OpenCVImage::Close()
    {
        image_.deallocate();
    }

    winrt::WinMLSamplesGalleryNative::OpenCVImage OpenCVImage::CreateFromPath(hstring const& path) {
        return winrt::make<OpenCVImage>(path);
    }
}
