#pragma once
#include "Common.h"

using namespace winrt::Microsoft::AI::MachineLearning;
using namespace winrt::Windows::Graphics::DirectX;
using namespace winrt::Windows::Graphics::Imaging;

enum class InputBindingType
{
    CPU,
    GPU
};
enum class InputDataType
{
    Tensor,
    ImageRGB,
    ImageBGR
};
enum class InputSourceType
{
    ImageFile,
    CSVFile,
    GeneratedData
};
enum class DeviceType
{
    CPU,
    DefaultGPU,
    MinPowerGPU,
    HighPerfGPU
};
enum class DeviceCreationLocation
{
    WinML,
    UserD3DDevice
};

class TypeHelper
{
public:
    static std::string Stringify(InputDataType inputDataType)
    {
        switch (inputDataType)
        {
            case InputDataType::Tensor:
                return "Tensor";
            case InputDataType::ImageRGB:
                return "RGB_Image";
            case InputDataType::ImageBGR:
                return "BGR_Image";
        }

        throw "No name found for this InputDataType";
    }

    static std::string Stringify(InputBindingType inputBindingType)
    {
        switch (inputBindingType)
        {
            case InputBindingType::CPU:
                return "CPU";
            case InputBindingType::GPU:
                return "GPU";
        }

        throw "No name found for this InputBindingType.";
    }

    static std::string Stringify(DeviceType deviceType)
    {
        switch (deviceType)
        {
            case DeviceType::CPU:
                return "CPU";
            case DeviceType::DefaultGPU:
                return "GPU";
            case DeviceType::MinPowerGPU:
                return "GPU_Min_Power";
            case DeviceType::HighPerfGPU:
                return "GPU_High_Performance";
        }

        throw "No name found for this DeviceType.";
    }

    static std::string Stringify(InputSourceType inputSourceType)
    {
        switch (inputSourceType)
        {
            case InputSourceType::ImageFile:
                return "Image_File";
            case InputSourceType::CSVFile:
                return "CSV_File";
            case InputSourceType::GeneratedData:
                return "Generated_Data";
        }

        throw "No name found for this DeviceType.";
    }

    static std::string Stringify(DeviceCreationLocation deviceCreationLocation)
    {
        switch (deviceCreationLocation)
        {
            case DeviceCreationLocation::UserD3DDevice:
                return "Client";
            case DeviceCreationLocation::WinML:
                return "WinML";
        }

        throw "No name found for this DeviceCreationLocation.";
    }

    static std::wstring Stringify(TensorKind tensorKind)
    {
        // IMPORTANT: This tensorKinds array needs to match the "enum class TensorKind" idl in
        // Windows.AI.MachineLearning.0.h
        switch (tensorKind)
        {
            case TensorKind::Undefined:
                return L"Undefined";
            case TensorKind::Float:
                return L"Float";
            case TensorKind::UInt8:
                return L"UInt8";
            case TensorKind::Int8:
                return L"Int8";
            case TensorKind::UInt16:
                return L"UInt16";
            case TensorKind::Int16:
                return L"Int16";
            case TensorKind::Int32:
                return L"Int32";
            case TensorKind::Int64:
                return L"Int64";
            case TensorKind::String:
                return L"String";
            case TensorKind::Boolean:
                return L"Boolean";
            case TensorKind::Float16:
                return L"Float16";
            case TensorKind::Double:
                return L"Double";
            case TensorKind::UInt32:
                return L"UInt32";
            case TensorKind::UInt64:
                return L"UInt64";
            case TensorKind::Complex64:
                return L"Complex64";
            case TensorKind::Complex128:
                return L"Complex128";
        };

        throw "No name found for this TensorKind.";
    }

    static LearningModelDeviceKind GetWinmlDeviceKind(DeviceType deviceType)
    {
        switch (deviceType)
        {
            case DeviceType::CPU:
                    return LearningModelDeviceKind::Cpu;
                case DeviceType::DefaultGPU:
                    return LearningModelDeviceKind::DirectX;
                case DeviceType::MinPowerGPU:
                    return LearningModelDeviceKind::DirectXMinPower;
                case DeviceType::HighPerfGPU:
                    return LearningModelDeviceKind::DirectXHighPerformance;
        }

        throw "No LearningModelDeviceKind found for this DeviceType.";
    }

    static BitmapPixelFormat GetBitmapPixelFormat(InputDataType inputDataType)
    {
        switch (inputDataType)
        {
            case InputDataType::ImageRGB:
                return BitmapPixelFormat::Rgba8;
            case InputDataType::ImageBGR:
                return BitmapPixelFormat::Bgra8;
        }

        throw "No BitmapPixelFormat found for this InputDataType.";
    }

    static DirectXPixelFormat GetDirectXPixelFormat(InputDataType inputDataType)
    {
        switch (inputDataType)
        {
            case InputDataType::ImageRGB:
                return DirectXPixelFormat::R8G8B8A8UInt;
            case InputDataType::ImageBGR:
                return DirectXPixelFormat::B8G8R8A8UIntNormalized;
        }

        throw "No DirectXPixelFormat found for this InputDataType.";
    }
};
