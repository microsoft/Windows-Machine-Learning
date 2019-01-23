#pragma once
#include "Common.h"

using namespace winrt::Windows::AI::MachineLearning;
using namespace winrt::Windows::Graphics::DirectX;
using namespace winrt::Windows::Graphics::Imaging;

enum class ImageDataType { ImageRGB, ImageBGR };

class TypeHelper
{
public:
    static BitmapPixelFormat GetBitmapPixelFormat(ImageDataType inputDataType)
    {
        switch (inputDataType)
        {
            case ImageDataType::ImageRGB: return BitmapPixelFormat::Rgba8;
            case ImageDataType::ImageBGR: return BitmapPixelFormat::Bgra8;
        }

        throw "No BitmapPixelFormat found for this ImageDataType.";
    }
};

