#pragma once
#include "Common.h"
#include "ModelBinding.h"

using namespace winrt::Windows::Media;
using namespace winrt::Windows::Storage;
using namespace winrt::Windows::AI::MachineLearning;

namespace BindingUtilities
{
    SoftwareBitmap LoadImageFile(const TensorFeatureDescriptor& imageDescriptor, ImageDataType inputDataType, const hstring& filePath)
    {
        // We assume NCHW and NCDHW
        uint64_t width = imageDescriptor.Shape().GetAt(imageDescriptor.Shape().Size() - 1);
        uint64_t height = imageDescriptor.Shape().GetAt(imageDescriptor.Shape().Size() - 2);
        uint64_t channelCount = imageDescriptor.Shape().GetAt(1);
        uint64_t batchCount = imageDescriptor.Shape().GetAt(0);

        try
        {
            // open the file
            StorageFile file = StorageFile::GetFileFromPathAsync(filePath).get();
            // get a stream on it
            auto stream = file.OpenAsync(FileAccessMode::Read).get();
            // Create the decoder from the stream
            BitmapDecoder decoder = BitmapDecoder::CreateAsync(stream).get();

            // If input dimensions are different from tensor input, then scale / crop while reading
            if (decoder.PixelHeight() != height ||
                   decoder.PixelWidth() != width)
            {
               
                // Create a transform object with default parameters (no transform)
                auto transform = BitmapTransform();
                transform.ScaledHeight(static_cast<uint32_t>(height));
                transform.ScaledWidth(static_cast<uint32_t>(width));
                transform.InterpolationMode(BitmapInterpolationMode::Cubic);

                // get the bitmap
                return decoder.GetSoftwareBitmapAsync(TypeHelper::GetBitmapPixelFormat(inputDataType),
                    BitmapAlphaMode::Ignore,
                    transform,
                    ExifOrientationMode::RespectExifOrientation,
                    ColorManagementMode::DoNotColorManage).get();
            }
            else
            {
                // get the bitmap
                return decoder.GetSoftwareBitmapAsync(TypeHelper::GetBitmapPixelFormat(inputDataType), BitmapAlphaMode::Ignore).get();
            }
        }
        catch (...)
        {
            std::cout << "BindingUtilities: could not open image file, make sure you are using fully qualified paths." << std::endl;
            return nullptr;
        }
    }

    std::vector<std::string> ReadCsvLine(std::ifstream& fileStream)
    {
        std::vector<std::string> elementStrings;
        // Read next line.
        std::string line;
        if (!std::getline(fileStream, line))
        {
            ThrowFailure(L"BindingUtilities: expected more input rows.");
        }

        // Split the line into strings for each value.
        std::istringstream elementsString(line);
        std::string elementString;
        while (std::getline(elementsString, elementString, ','))
        {
            elementStrings.push_back(elementString);
        }
        return elementStrings;
    }

    template <typename T>
    void WriteDataToBinding(const std::vector<std::string>& elementStrings, ModelBinding<T>& binding)
    {
        /*if (binding.GetDataBufferSize() != elementStrings.size())
        {
            throw hresult_invalid_argument(L"CSV Input is size/shape is different from what model expects");
        }*/
        T* data = binding.GetData();
        for (const auto &elementString : elementStrings)
        {
            T value;
            std::stringstream(elementString) >> value;
            *data = value;
            data++;
        }
    }

    std::vector<std::string> ParseCSVElementStrings(const std::wstring& csvFilePath)
    {
        std::ifstream fileStream;
        fileStream.open(csvFilePath);
        if (!fileStream.is_open())
        {
            ThrowFailure(L"BindingUtilities: could not open data file.");
        }

        std::vector<std::string> elementStrings = ReadCsvLine(fileStream);

        return elementStrings;
    }

    // Binds tensor floats, ints, doubles from CSV data.
    ITensor CreateBindableTensor(const ILearningModelFeatureDescriptor& description, std::wstring inputPath)
    {
        auto name = description.Name();
        auto tensorDescriptor = description.try_as<TensorFeatureDescriptor>();

        if (!tensorDescriptor)
        {
            std::cout << "BindingUtilities: Input Descriptor type isn't tensor." << std::endl;
            throw;
        }

        std::vector<std::string> elementStrings;
        switch (tensorDescriptor.TensorKind())
        {
            case TensorKind::Undefined:
            {
                std::cout << "BindingUtilities: TensorKind is undefined." << std::endl;
                throw hresult_invalid_argument();
            }
            case TensorKind::Float:
            {
                ModelBinding<float> binding(description);
                
                elementStrings = ParseCSVElementStrings(inputPath);
                WriteDataToBinding<float>(elementStrings, binding);
                return TensorFloat::CreateFromArray(binding.GetShapeBuffer(), binding.GetDataBuffer());
            }
            break;
            case TensorKind::Float16:
            {
                ModelBinding<float> binding(description);
                elementStrings = ParseCSVElementStrings(inputPath);
                WriteDataToBinding<float>(elementStrings, binding);
                return TensorFloat16Bit::CreateFromArray(binding.GetShapeBuffer(), binding.GetDataBuffer());
            }
            break;
            case TensorKind::Double:
            {
                ModelBinding<double> binding(description);
                elementStrings = ParseCSVElementStrings(inputPath);
                WriteDataToBinding<double>(elementStrings, binding);
                return TensorDouble::CreateFromArray(binding.GetShapeBuffer(), binding.GetDataBuffer());
            }
            break;
            case TensorKind::Int8:
            {
                ModelBinding<uint8_t> binding(description);
                elementStrings = ParseCSVElementStrings(inputPath);
                WriteDataToBinding<uint8_t>(elementStrings, binding);
                return TensorInt8Bit::CreateFromArray(binding.GetShapeBuffer(), binding.GetDataBuffer());
            }
            break;
            case TensorKind::UInt8:
            {
                ModelBinding<uint8_t> binding(description);
                elementStrings = ParseCSVElementStrings(inputPath);
                WriteDataToBinding<uint8_t>(elementStrings, binding);
                return TensorUInt8Bit::CreateFromArray(binding.GetShapeBuffer(), binding.GetDataBuffer());
            }
            break;
            case TensorKind::Int16:
            {
                ModelBinding<int16_t> binding(description);
                elementStrings = ParseCSVElementStrings(inputPath);
                WriteDataToBinding<int16_t>(elementStrings, binding);
                return TensorInt16Bit::CreateFromArray(binding.GetShapeBuffer(), binding.GetDataBuffer());
            }
            break;
            case TensorKind::UInt16:
            {
                ModelBinding<uint16_t> binding(description);
                elementStrings = ParseCSVElementStrings(inputPath);
                WriteDataToBinding<uint16_t>(elementStrings, binding);
				return TensorUInt16Bit::CreateFromArray(binding.GetShapeBuffer(), binding.GetDataBuffer());
            }
            break;
            case TensorKind::Int32:
            {
                ModelBinding<int32_t> binding(description);
                elementStrings = ParseCSVElementStrings(inputPath);
                WriteDataToBinding<int32_t>(elementStrings, binding);
                return TensorInt32Bit::CreateFromArray(binding.GetShapeBuffer(), binding.GetDataBuffer());
            }
            break;
            case TensorKind::UInt32:
            {
                ModelBinding<uint32_t> binding(description);
                elementStrings = ParseCSVElementStrings(inputPath);
                WriteDataToBinding<uint32_t>(elementStrings, binding);
                return TensorUInt32Bit::CreateFromArray(binding.GetShapeBuffer(), binding.GetDataBuffer());
            }
            break;
            case TensorKind::Int64:
            {
                ModelBinding<int64_t> binding(description);
                elementStrings = ParseCSVElementStrings(inputPath);
                WriteDataToBinding<int64_t>(elementStrings, binding);
                return TensorInt64Bit::CreateFromArray(binding.GetShapeBuffer(), binding.GetDataBuffer());
            }
            break;
            case TensorKind::UInt64:
            {
                ModelBinding<uint64_t> binding(description);
                elementStrings = ParseCSVElementStrings(inputPath);
                WriteDataToBinding<uint64_t>(elementStrings, binding);
                return TensorUInt64Bit::CreateFromArray(binding.GetShapeBuffer(), binding.GetDataBuffer());
            }
            break;
        }

        std::cout << "BindingUtilities: TensorKind has not been implemented." << std::endl;
        throw hresult_not_implemented();
    }

    ImageFeatureValue CreateBindableImage(
        const ILearningModelFeatureDescriptor&
        featureDescriptor,
        const std::wstring& imagePath,
        ImageDataType inputDataType	)
    {
        auto imageDescriptor = featureDescriptor.try_as<TensorFeatureDescriptor>();

        if (!imageDescriptor)
        {
            std::cout << "BindingUtilities: Input Descriptor type isn't tensor." << std::endl;
            throw;
        }

        auto softwareBitmap = LoadImageFile(imageDescriptor, inputDataType, imagePath.c_str());

		auto videoFrame = VideoFrame::CreateWithSoftwareBitmap(softwareBitmap);

        return ImageFeatureValue::CreateFromVideoFrame(videoFrame);
    }
 };
