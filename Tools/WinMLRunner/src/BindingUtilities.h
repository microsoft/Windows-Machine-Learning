#pragma once
#include <random>
#include <time.h>
#include "Common.h"
#include "ModelBinding.h"
#include "Windows.AI.Machinelearning.Native.h"

using namespace winrt::Windows::Media;
using namespace winrt::Windows::Storage;
using namespace winrt::Windows::AI::MachineLearning;
using namespace winrt::Windows::Foundation::Collections;
using namespace winrt::Windows::Graphics::DirectX;
using namespace winrt::Windows::Graphics::Imaging;
using namespace winrt::Windows::Graphics::DirectX::Direct3D11;

namespace BindingUtilities
{
    static unsigned int seed = 0;
    static std::independent_bits_engine<std::default_random_engine, CHAR_BIT, unsigned int> randomBitsEngine;

    SoftwareBitmap GenerateGarbageImage(const TensorFeatureDescriptor& imageDescriptor, InputDataType inputDataType)
    {
        assert(inputDataType != InputDataType::Tensor);

        // We assume NCHW and NCDHW
        uint64_t width = imageDescriptor.Shape().GetAt(imageDescriptor.Shape().Size() - 1);
        uint64_t height = imageDescriptor.Shape().GetAt(imageDescriptor.Shape().Size() - 2);
        uint64_t channelCount = imageDescriptor.Shape().GetAt(1);
        uint64_t batchCount = imageDescriptor.Shape().GetAt(0);

        // If the batchCount is infinite, we can put as many images as we want
        if (batchCount >= ULLONG_MAX)
        {
            batchCount = 3;
        }

        // We have to create RGBA8 or BGRA8 images, so we need 4 channels
        uint32_t totalByteSize = static_cast<uint32_t>(width) * static_cast<uint32_t>(height) * 4;

        // Generate values for the image based on a seed
        std::vector<uint8_t> data(totalByteSize);
        randomBitsEngine.seed(seed++);
        std::generate(data.begin(), data.end(), randomBitsEngine);

        // Write the values to a buffer
        winrt::array_view<const uint8_t> dataView(data);
        InMemoryRandomAccessStream dataStream;
        DataWriter dataWriter(dataStream);
        dataWriter.WriteBytes(dataView);
        IBuffer buffer = dataWriter.DetachBuffer();

        // Create the software bitmap
        return SoftwareBitmap::CreateCopyFromBuffer(buffer, TypeHelper::GetBitmapPixelFormat(inputDataType), static_cast<int32_t>(width), static_cast<int32_t>(height));
    }

    SoftwareBitmap LoadImageFile(const TensorFeatureDescriptor& imageDescriptor, InputDataType inputDataType, const hstring& filePath, const CommandLineArgs& args, uint32_t iterationNum)
    {
        assert(inputDataType != InputDataType::Tensor);

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
            if (args.IsAutoScale() &&
                 ( decoder.PixelHeight() != height ||
                   decoder.PixelWidth() != width))
            {
                if (!args.TerseOutput() || iterationNum == 0)
                    std::cout << std::endl << "Binding Utilities: AutoScaling input image to match model input dimensions...";

                // Create a transform object with default parameters (no transform)
                auto transform = BitmapTransform();
                transform.ScaledHeight(static_cast<uint32_t>(height));
                transform.ScaledWidth(static_cast<uint32_t>(width));
                transform.InterpolationMode(args.AutoScaleInterpMode());

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

    VideoFrame CreateVideoFrame(const SoftwareBitmap& softwareBitmap, InputBindingType inputBindingType, InputDataType inputDataType, const IDirect3DDevice winrtDevice)
    {
        VideoFrame inputImage = VideoFrame::CreateWithSoftwareBitmap(softwareBitmap);

        if (inputBindingType == InputBindingType::GPU)
        {
            VideoFrame gpuImage = winrtDevice
                ? VideoFrame::CreateAsDirect3D11SurfaceBacked(TypeHelper::GetDirectXPixelFormat(inputDataType), softwareBitmap.PixelWidth(), softwareBitmap.PixelHeight(), winrtDevice)
                : VideoFrame::CreateAsDirect3D11SurfaceBacked(TypeHelper::GetDirectXPixelFormat(inputDataType), softwareBitmap.PixelWidth(), softwareBitmap.PixelHeight());

            inputImage.CopyToAsync(gpuImage).get();

            return gpuImage;
        }

        return inputImage;
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
        if (binding.GetDataBufferSize() != elementStrings.size())
        {
            throw hresult_invalid_argument(L"CSV Input is size/shape is different from what model expects");
        }
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
    ITensor CreateBindableTensor(const ILearningModelFeatureDescriptor& description, const CommandLineArgs& args)
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
                if (args.IsGarbageInput())
                {
                    memset(binding.GetData(), 0, sizeof(float) * binding.GetDataBufferSize());
                }
                else
                {
                    elementStrings = ParseCSVElementStrings(args.CsvPath());
                    WriteDataToBinding<float>(elementStrings, binding);
                }
                return TensorFloat::CreateFromArray(binding.GetShapeBuffer(), binding.GetDataBuffer());
            }
            break;
            case TensorKind::Float16:
            {
                ModelBinding<float> binding(description);
                if (args.IsGarbageInput())
                {
                    memset(binding.GetData(), 0, sizeof(float) * binding.GetDataBufferSize());
                }
                else
                {
                    elementStrings = ParseCSVElementStrings(args.CsvPath());
                    WriteDataToBinding<float>(elementStrings, binding);
                }
                return TensorFloat16Bit::CreateFromArray(binding.GetShapeBuffer(), binding.GetDataBuffer());
            }
            break;
            case TensorKind::Double:
            {
                ModelBinding<double> binding(description);
                if (args.IsGarbageInput())
                {
                    memset(binding.GetData(), 0, sizeof(double) * binding.GetDataBufferSize());
                }
                else
                {
                    elementStrings = ParseCSVElementStrings(args.CsvPath());
                    WriteDataToBinding<double>(elementStrings, binding);
                }
                return TensorDouble::CreateFromArray(binding.GetShapeBuffer(), binding.GetDataBuffer());
            }
            break;
            case TensorKind::Int8:
            {
                ModelBinding<uint8_t> binding(description);
                if (args.IsGarbageInput())
                {
                    memset(binding.GetData(), 0, sizeof(uint8_t) * binding.GetDataBufferSize());
                }
                else
                {
                    elementStrings = ParseCSVElementStrings(args.CsvPath());
                    WriteDataToBinding<uint8_t>(elementStrings, binding);
                }
                return TensorInt8Bit::CreateFromArray(binding.GetShapeBuffer(), binding.GetDataBuffer());
            }
            break;
            case TensorKind::UInt8:
            {
                ModelBinding<uint8_t> binding(description);
                if (args.IsGarbageInput())
                {
                    memset(binding.GetData(), 0, sizeof(uint8_t) * binding.GetDataBufferSize());
                }
                else
                {
                    elementStrings = ParseCSVElementStrings(args.CsvPath());
                    WriteDataToBinding<uint8_t>(elementStrings, binding);
                }
                return TensorUInt8Bit::CreateFromArray(binding.GetShapeBuffer(), binding.GetDataBuffer());
            }
            break;
            case TensorKind::Int16:
            {
                ModelBinding<int16_t> binding(description);
                if (args.IsGarbageInput())
                {
                    memset(binding.GetData(), 0, sizeof(int16_t) * binding.GetDataBufferSize());
                }
                else
                {
                    elementStrings = ParseCSVElementStrings(args.CsvPath());
                    WriteDataToBinding<int16_t>(elementStrings, binding);
                }
                return TensorInt16Bit::CreateFromArray(binding.GetShapeBuffer(), binding.GetDataBuffer());
            }
            break;
            case TensorKind::UInt16:
            {
                ModelBinding<uint16_t> binding(description);
                if (args.IsGarbageInput())
                {
                    memset(binding.GetData(), 0, sizeof(uint16_t) * binding.GetDataBufferSize());
                }
                else
                {
                    elementStrings = ParseCSVElementStrings(args.CsvPath());
                    WriteDataToBinding<uint16_t>(elementStrings, binding);
                }
                return TensorUInt16Bit::CreateFromArray(binding.GetShapeBuffer(), binding.GetDataBuffer());
            }
            break;
            case TensorKind::Int32:
            {
                ModelBinding<int32_t> binding(description);
                if (args.IsGarbageInput())
                {
                    memset(binding.GetData(), 0, sizeof(int32_t) * binding.GetDataBufferSize());
                }
                else
                {
                    elementStrings = ParseCSVElementStrings(args.CsvPath());
                    WriteDataToBinding<int32_t>(elementStrings, binding);
                }
                return TensorInt32Bit::CreateFromArray(binding.GetShapeBuffer(), binding.GetDataBuffer());
            }
            break;
            case TensorKind::UInt32:
            {
                ModelBinding<uint32_t> binding(description);
                if (args.IsGarbageInput())
                {
                    memset(binding.GetData(), 0, sizeof(uint32_t) * binding.GetDataBufferSize());
                }
                else
                {
                    elementStrings = ParseCSVElementStrings(args.CsvPath());
                    WriteDataToBinding<uint32_t>(elementStrings, binding);
                }
                return TensorUInt32Bit::CreateFromArray(binding.GetShapeBuffer(), binding.GetDataBuffer());
            }
            break;
            case TensorKind::Int64:
            {
                ModelBinding<int64_t> binding(description);
                if (args.IsGarbageInput())
                {
                    memset(binding.GetData(), 0, sizeof(int64_t) * binding.GetDataBufferSize());
                }
                else
                {
                    elementStrings = ParseCSVElementStrings(args.CsvPath());
                    WriteDataToBinding<int64_t>(elementStrings, binding);
                }
                return TensorInt64Bit::CreateFromArray(binding.GetShapeBuffer(), binding.GetDataBuffer());
            }
            break;
            case TensorKind::UInt64:
            {
                ModelBinding<uint64_t> binding(description);
                if (args.IsGarbageInput())
                {
                    memset(binding.GetData(), 0, sizeof(uint64_t) * binding.GetDataBufferSize());
                }
                else
                {
                    elementStrings = ParseCSVElementStrings(args.CsvPath());
                    WriteDataToBinding<uint64_t>(elementStrings, binding);
                }
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
        InputBindingType inputBindingType,
        InputDataType inputDataType,
        const IDirect3DDevice winrtDevice,
        const CommandLineArgs& args,
        uint32_t iterationNum
    )
    {
        auto imageDescriptor = featureDescriptor.try_as<TensorFeatureDescriptor>();

        if (!imageDescriptor)
        {
            std::cout << "BindingUtilities: Input Descriptor type isn't tensor." << std::endl;
            throw;
        }

        auto softwareBitmap = imagePath.empty()
            ? GenerateGarbageImage(imageDescriptor, inputDataType)
            : LoadImageFile(imageDescriptor, inputDataType, imagePath.c_str(), args, iterationNum);

        auto videoFrame = CreateVideoFrame(softwareBitmap, inputBindingType, inputDataType, winrtDevice);

        return ImageFeatureValue::CreateFromVideoFrame(videoFrame);
    }

    template<typename K, typename V>
    void OutputSequenceBinding(IMapView<hstring, winrt::Windows::Foundation::IInspectable> results, hstring name)
    {
        auto map = results.Lookup(name).as<IVectorView<IMap<K, V>>>().GetAt(0);
        auto iter = map.First();

        K maxKey = -1;
        V maxVal = -1;

        while (iter.HasCurrent())
        {
            auto pair = iter.Current();
            if (pair.Value() > maxKey)
            {
                maxVal = pair.Value();
                maxKey = pair.Key();
            }
            iter.MoveNext();
        }
        std::cout << " " << maxKey << " " << maxVal << std::endl;
    }

    void PrintOrSaveEvaluationResults(const LearningModel& model,
                                      const CommandLineArgs& args,
                                      const IMapView<hstring, winrt::Windows::Foundation::IInspectable>& results,
                                      OutputHelper& output,
                                      int iterationNum)
    {
        for (auto&& desc : model.OutputFeatures())
        {
            if (desc.Kind() == LearningModelFeatureKind::Tensor)
            {
                std::string name = to_string(desc.Name());
                TensorFeatureDescriptor tensorDescriptor = desc.as<TensorFeatureDescriptor>();
                TensorKind tensorKind = tensorDescriptor.TensorKind();
                switch (tensorKind)
                {
                case TensorKind::String:
                {
                    if (!args.IsGarbageInput())
                    {
                        auto resultVector = results.Lookup(desc.Name()).as<TensorString>().GetAsVectorView();
                        auto output = resultVector.GetAt(0).data();
                        std::wcout << " Result: " << output << std::endl;
                    }
                }
                break;
                case TensorKind::Float16:
                {
                    com_ptr<ITensorNative> itn = results.Lookup(desc.Name()).as<ITensorNative>();
                    output.ProcessFP16TensorResult(itn, iterationNum, args, name);
                }
                break;
                case TensorKind::Float:
                {
                    com_ptr<ITensorNative> itn = results.Lookup(desc.Name()).as<ITensorNative>();
                    output.ProcessTensorResult<float>(itn, iterationNum, args, name);
                }
                break;
                case TensorKind::Int64:
                {
                    auto resultVector = results.Lookup(desc.Name()).as<TensorInt64Bit>().GetAsVectorView();
                    if (!args.IsGarbageInput())
                    {
                        auto output = resultVector.GetAt(0);
                        std::wcout << " Result: " << output << std::endl;
                    }
                }
                break;
                default:
                {
                    std::cout << "BindingUtilities: output type not implemented.";
                }
                break;
                }
                std::cout << std::endl;
            }
            else if (desc.Kind() == LearningModelFeatureKind::Sequence)
            {
                auto seqDescriptor = desc.as<SequenceFeatureDescriptor>();
                auto mapDescriptor = seqDescriptor.ElementDescriptor().as<MapFeatureDescriptor>();
                auto keyKind = mapDescriptor.KeyKind();
                auto valueKind = mapDescriptor.ValueDescriptor();
                auto tensorKind = valueKind.as<TensorFeatureDescriptor>().TensorKind();
                switch (keyKind)
                {
                case TensorKind::Int64:
                {
                    OutputSequenceBinding<int64_t, float>(results, desc.Name());
                }
                break;
                case TensorKind::Float:
                {
                    OutputSequenceBinding<float, float>(results, desc.Name());
                }
                break;
                }
            }
        }
    }
 };
