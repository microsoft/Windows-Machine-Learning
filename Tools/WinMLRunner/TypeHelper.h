#pragma once
#include "Common.h"

using namespace winrt::Windows::AI::MachineLearning;
using namespace winrt::Windows::Graphics::DirectX;
using namespace winrt::Windows::Graphics::Imaging;

enum class InputBindingType { CPU, GPU };
enum class InputDataType { Tensor, ImageRGB, ImageBGR };
enum class InputSourceType { ImageFile, CSVFile, GeneratedData };
enum class DeviceType { CPU, DefaultGPU, MinPowerGPU, HighPerfGPU };
enum class DeviceCreationLocation { WinML, ClientCode };

class TypeHelper
{
public:
    static std::string Stringify(InputDataType inputDataType)
    {
        switch (inputDataType)
        {
            case InputDataType::Tensor: return "Tensor";
            case InputDataType::ImageRGB: return "RGB Image";
            case InputDataType::ImageBGR: return "BGR Image";
        }

        throw "No name found for this InputDataType";
    }

    static std::string Stringify(InputBindingType inputBindingType)
    {
        switch (inputBindingType)
        {
            case InputBindingType::CPU: return "CPU";
            case InputBindingType::GPU: return "GPU";
        }

        throw "No name found for this InputBindingType.";
    }

    static std::string Stringify(DeviceType deviceType)
    {
        switch (deviceType)
        {
            case DeviceType::CPU: return "CPU";
            case DeviceType::DefaultGPU: return "GPU";
            case DeviceType::MinPowerGPU: return "GPU (min power)";
            case DeviceType::HighPerfGPU: return "GPU (high perf)";
        }

        throw "No name found for this DeviceType.";
    }

    static std::string Stringify(InputSourceType inputSourceType)
    {
        switch (inputSourceType)
        {
            case InputSourceType::ImageFile: return "Image File";
            case InputSourceType::CSVFile: return "CSV File";
            case InputSourceType::GeneratedData: return "Generated Data";
        }

        throw "No name found for this DeviceType.";
    }

    static std::string Stringify(DeviceCreationLocation deviceCreationLocation)
    {
        switch (deviceCreationLocation)
        {
            case DeviceCreationLocation::ClientCode: return "Client Code";
            case DeviceCreationLocation::WinML: return "WinML";
        }

        throw "No name found for this DeviceCreationLocation.";
    }

    static LearningModelDeviceKind GetWinmlDeviceKind(DeviceType deviceType)
    {
        switch (deviceType)
        {
            case DeviceType::CPU: return LearningModelDeviceKind::Cpu;
            case DeviceType::DefaultGPU: return LearningModelDeviceKind::DirectX;
            case DeviceType::MinPowerGPU: return LearningModelDeviceKind::DirectXMinPower;
            case DeviceType::HighPerfGPU: return LearningModelDeviceKind::DirectXHighPerformance;
        }

        throw "No LearningModelDeviceKind found for this DeviceType.";
    }

    static BitmapPixelFormat GetBitmapPixelFormat(InputDataType inputDataType)
    {
        switch (inputDataType)
        {
            case InputDataType::ImageRGB: return BitmapPixelFormat::Rgba8;
            case InputDataType::ImageBGR: return BitmapPixelFormat::Bgra8;
        }

        throw "No BitmapPixelFormat found for this InputDataType.";
    }

    static DirectXPixelFormat GetDirectXPixelFormat(InputDataType inputDataType)
    {
        switch (inputDataType)
        {
            case InputDataType::ImageRGB: return DirectXPixelFormat::R8G8B8A8UInt;
            case InputDataType::ImageBGR: return DirectXPixelFormat::B8G8R8A8UIntNormalized;
        }

        throw "No DirectXPixelFormat found for this InputDataType.";
    }
};

