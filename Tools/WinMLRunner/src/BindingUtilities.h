#pragma once
#include <random>
#include <time.h>
#include "Common.h"
#include "Windows.AI.Machinelearning.Native.h"

using namespace winrt::Windows::Media;
using namespace winrt::Windows::Storage;
using namespace winrt::Windows::AI::MachineLearning;
using namespace winrt::Windows::Foundation::Collections;
using namespace winrt::Windows::Graphics::DirectX;
using namespace winrt::Windows::Graphics::Imaging;
using namespace winrt::Windows::Graphics::DirectX::Direct3D11;

template <TensorKind T> struct TensorKindToType
{
    static_assert(true, "No TensorKind mapped for given type!");
};
template <> struct TensorKindToType<TensorKind::UInt8>
{
    typedef uint8_t Type;
};
template <> struct TensorKindToType<TensorKind::Int8>
{
    typedef uint8_t Type;
};
template <> struct TensorKindToType<TensorKind::UInt16>
{
    typedef uint16_t Type;
};
template <> struct TensorKindToType<TensorKind::Int16>
{
    typedef int16_t Type;
};
template <> struct TensorKindToType<TensorKind::UInt32>
{
    typedef uint32_t Type;
};
template <> struct TensorKindToType<TensorKind::Int32>
{
    typedef int32_t Type;
};
template <> struct TensorKindToType<TensorKind::UInt64>
{
    typedef uint64_t Type;
};
template <> struct TensorKindToType<TensorKind::Int64>
{
    typedef int64_t Type;
};
template <> struct TensorKindToType<TensorKind::Boolean>
{
    typedef boolean Type;
};
template <> struct TensorKindToType<TensorKind::Double>
{
    typedef double Type;
};
template <> struct TensorKindToType<TensorKind::Float>
{
    typedef float Type;
};
template <> struct TensorKindToType<TensorKind::Float16>
{
    typedef HALF Type;
};
template <> struct TensorKindToType<TensorKind::String>
{
    typedef winrt::hstring Type;
};

template <TensorKind T> struct TensorKindToValue
{
    static_assert(true, "No TensorKind mapped for given type!");
};
template <> struct TensorKindToValue<TensorKind::UInt8>
{
    typedef TensorUInt8Bit Type;
};
template <> struct TensorKindToValue<TensorKind::Int8>
{
    typedef TensorInt8Bit Type;
};
template <> struct TensorKindToValue<TensorKind::UInt16>
{
    typedef TensorUInt16Bit Type;
};
template <> struct TensorKindToValue<TensorKind::Int16>
{
    typedef TensorInt16Bit Type;
};
template <> struct TensorKindToValue<TensorKind::UInt32>
{
    typedef TensorUInt32Bit Type;
};
template <> struct TensorKindToValue<TensorKind::Int32>
{
    typedef TensorInt32Bit Type;
};
template <> struct TensorKindToValue<TensorKind::UInt64>
{
    typedef TensorUInt64Bit Type;
};
template <> struct TensorKindToValue<TensorKind::Int64>
{
    typedef TensorInt64Bit Type;
};
template <> struct TensorKindToValue<TensorKind::Boolean>
{
    typedef TensorBoolean Type;
};
template <> struct TensorKindToValue<TensorKind::Double>
{
    typedef TensorDouble Type;
};
template <> struct TensorKindToValue<TensorKind::Float>
{
    typedef TensorFloat Type;
};
template <> struct TensorKindToValue<TensorKind::Float16>
{
    typedef TensorFloat16Bit Type;
};
template <> struct TensorKindToValue<TensorKind::String>
{
    typedef TensorString Type;
};

void GetHeightAndWidthFromLearningModelFeatureDescriptor(const ILearningModelFeatureDescriptor& modelFeatureDescriptor,
    uint64_t& width, uint64_t& height)
{
    if (modelFeatureDescriptor.Kind() == LearningModelFeatureKind::Tensor)
    {
        // We assume NCHW
        auto tensorDescriptor = modelFeatureDescriptor.try_as<TensorFeatureDescriptor>();
        if (tensorDescriptor.Shape().Size() != 4)
        {
            throw hresult_invalid_argument(L"Cannot generate arbitrary image for tensor input of dimensions: " +
                                           tensorDescriptor.Shape().Size());
        }
        height = tensorDescriptor.Shape().GetAt(2);
        width = tensorDescriptor.Shape().GetAt(3);
    }
    else if (modelFeatureDescriptor.Kind() == LearningModelFeatureKind::Image)
    {
        auto imageDescriptor = modelFeatureDescriptor.try_as<IImageFeatureDescriptor>();
        height = imageDescriptor.Height();
        width = imageDescriptor.Width();
    }
    else
    {
        throw hresult_not_implemented(
            L"Generating arbitrary image not supported for input types that aren't tensor or image.");
    }
}

namespace BindingUtilities
{
    static unsigned int seed = 0;
    static std::independent_bits_engine<std::default_random_engine, CHAR_BIT, unsigned int> randomBitsEngine;

    SoftwareBitmap GenerateGarbageImage(const ILearningModelFeatureDescriptor& modelFeatureDescriptor, InputDataType inputDataType)
    {
        assert(inputDataType != InputDataType::Tensor);
        uint64_t width = 0;
        uint64_t height = 0;
        GetHeightAndWidthFromLearningModelFeatureDescriptor(modelFeatureDescriptor, width, height);

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
        return SoftwareBitmap::CreateCopyFromBuffer(buffer, TypeHelper::GetBitmapPixelFormat(inputDataType),
                                                    static_cast<int32_t>(width), static_cast<int32_t>(height));
    }

    SoftwareBitmap LoadImageFile(const ILearningModelFeatureDescriptor& modelFeatureDescriptor,
                                 InputDataType inputDataType,
                                 const hstring& filePath, const CommandLineArgs& args, uint32_t iterationNum)
    {
        assert(inputDataType != InputDataType::Tensor);

        // We assume NCHW and NCDHW
        uint64_t width = 0;
        uint64_t height = 0;
        GetHeightAndWidthFromLearningModelFeatureDescriptor(modelFeatureDescriptor, width, height);
        try
        {
            // open the file
            StorageFile file = StorageFile::GetFileFromPathAsync(filePath).get();
            // get a stream on it
            auto stream = file.OpenAsync(FileAccessMode::Read).get();
            // Create the decoder from the stream
            BitmapDecoder decoder = BitmapDecoder::CreateAsync(stream).get();

            // If input dimensions are different from tensor input, then scale / crop while reading
            if (args.IsAutoScale() && (decoder.PixelHeight() != height || decoder.PixelWidth() != width))
            {
                if (!args.TerseOutput() || iterationNum == 0)
                    std::cout << std::endl
                              << "Binding Utilities: AutoScaling input image to match model input dimensions...";

                // Create a transform object with default parameters (no transform)
                auto transform = BitmapTransform();
                transform.ScaledHeight(static_cast<uint32_t>(height));
                transform.ScaledWidth(static_cast<uint32_t>(width));
                transform.InterpolationMode(args.AutoScaleInterpMode());

                // get the bitmap
                return decoder
                    .GetSoftwareBitmapAsync(TypeHelper::GetBitmapPixelFormat(inputDataType), BitmapAlphaMode::Ignore,
                                            transform, ExifOrientationMode::RespectExifOrientation,
                                            ColorManagementMode::DoNotColorManage)
                    .get();
            }
            else
            {
                // get the bitmap
                return decoder
                    .GetSoftwareBitmapAsync(TypeHelper::GetBitmapPixelFormat(inputDataType), BitmapAlphaMode::Ignore)
                    .get();
            }
        }
        catch (...)
        {
            std::cout << "BindingUtilities: could not open image file, make sure you are using fully qualified paths."
                      << std::endl;
            return nullptr;
        }
    }

    VideoFrame CreateVideoFrame(const SoftwareBitmap& softwareBitmap, InputBindingType inputBindingType,
                                InputDataType inputDataType, const IDirect3DDevice winrtDevice)
    {
        VideoFrame inputImage = VideoFrame::CreateWithSoftwareBitmap(softwareBitmap);

        if (inputBindingType == InputBindingType::GPU)
        {
            VideoFrame gpuImage =
                winrtDevice
                    ? VideoFrame::CreateAsDirect3D11SurfaceBacked(TypeHelper::GetDirectXPixelFormat(inputDataType),
                                                                  softwareBitmap.PixelWidth(),
                                                                  softwareBitmap.PixelHeight(), winrtDevice)
                    : VideoFrame::CreateAsDirect3D11SurfaceBacked(TypeHelper::GetDirectXPixelFormat(inputDataType),
                                                                  softwareBitmap.PixelWidth(),
                                                                  softwareBitmap.PixelHeight());

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
    void WriteDataToBinding(const std::vector<std::string>& elementStrings, T* bindingMemory,
                            uint32_t bindingMemorySize)
    {
        if (bindingMemorySize / sizeof(T) != elementStrings.size())
        {
            throw hresult_invalid_argument(L"CSV Input is size/shape is different from what model expects");
        }
        T* data = bindingMemory;
        for (const auto& elementString : elementStrings)
        {
            float value;
            std::stringstream(elementString) >> value;
            if (!std::is_same<T, HALF>::value)
            {
                *data = static_cast<T>(value);
            }
            else
            {
                *reinterpret_cast<HALF*>(data) = XMConvertFloatToHalf(value);
            }
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

    template <TensorKind T>
    static ITensor CreateTensor(const CommandLineArgs& args, std::vector<std::string>& tensorStringInput,
                                IVectorView<int64_t> tensorShape)
    {
        using TensorValue = typename TensorKindToValue<T>::Type;
        using DataType = typename TensorKindToType<T>::Type;
        std::vector<int64_t> vecShape = {};
        for (UINT dim = 0; dim < tensorShape.Size(); dim++)
        {
            INT64 dimSize = tensorShape.GetAt(dim);
            if (dimSize > 0) // If the dimension is greater than 0, then it is known.
            {
                vecShape.push_back(dimSize);
            }
            else // otherwise, make sure that the dimension is -1, representing free dimension. If not, then it's an
                 // invalid model.
            {
                if (dimSize == -1)
                {
                    vecShape.push_back(1);
                }
                else
                {
                    throw hresult_invalid_argument(L"Failed to create a tensor with an unknown dimension of: " +
                                                   dimSize);
                }
            }
        }
        auto tensorValue = TensorValue::Create(vecShape);

        com_ptr<ITensorNative> spTensorValueNative;
        tensorValue.as(spTensorValueNative);

        BYTE* actualData;
        uint32_t actualSizeInBytes;
        spTensorValueNative->GetBuffer(&actualData,
                                       &actualSizeInBytes); // Need to GetBuffer to have CPU memory backing tensorValue

        if (!args.CsvPath().empty())
        {
            WriteDataToBinding<DataType>(tensorStringInput, reinterpret_cast<DataType*>(actualData), actualSizeInBytes);
        }
        else if (args.IsGarbageInput())
        {
            return tensorValue;
        }
        else
        {
            // Creating Tensors for Input Images haven't been added yet.
            throw hresult_not_implemented(L"Creating Tensors for Input Images haven't been implemented yet!");
        }
        return tensorValue;
    }

    // Binds tensor floats, ints, doubles from CSV data.
    ITensor CreateBindableTensor(const ILearningModelFeatureDescriptor& description, const CommandLineArgs& args)
    {
        std::vector<std::string> elementStrings;
        if (!args.CsvPath().empty())
        {
            elementStrings = ParseCSVElementStrings(args.CsvPath());
        }

        // Try Image Feature Descriptor
        auto imageFeatureDescriptor = description.try_as<ImageFeatureDescriptor>();
        if (imageFeatureDescriptor)
        {
            int64_t channels;
            if (imageFeatureDescriptor.BitmapPixelFormat() == BitmapPixelFormat::Gray16 ||
                imageFeatureDescriptor.BitmapPixelFormat() == BitmapPixelFormat::Gray8)
            {
                channels = 1;
            }
            else if (imageFeatureDescriptor.BitmapPixelFormat() == BitmapPixelFormat::Bgra8 ||
                imageFeatureDescriptor.BitmapPixelFormat() == BitmapPixelFormat::Rgba16 ||
                imageFeatureDescriptor.BitmapPixelFormat() == BitmapPixelFormat::Rgba8)
            {
                channels = 3;
            }
            else
            {
                throw hresult_not_implemented(L"BitmapPixel format not yet handled by WinMLRunner.");
            }
            std::vector<int64_t> shape = { 1, channels, imageFeatureDescriptor.Height(), imageFeatureDescriptor.Width() };
            IVectorView<int64_t> shapeVectorView = single_threaded_vector(std::move(shape)).GetView();
            return CreateTensor<TensorKind::Float>(args, elementStrings, shapeVectorView);
        }
        else
        {
            std::cout << "BindingUtilities: Input Descriptor type haisn't image feature descriptor. Attempting to "
                         "interpret as tensor feature descriptor.."
                      << std::endl;
        }

        auto tensorDescriptor = description.try_as<TensorFeatureDescriptor>();
        if (!tensorDescriptor)
        {
            std::cout << "BindingUtilities: Input Descriptor type isn't tensor." << std::endl;
            throw;
        }
        switch (tensorDescriptor.TensorKind())
        {
            case TensorKind::Undefined:
            {
                std::cout << "BindingUtilities: TensorKind is undefined." << std::endl;
                throw hresult_invalid_argument();
            }
            case TensorKind::Float:
            {
                return CreateTensor<TensorKind::Float>(args, elementStrings, tensorDescriptor.Shape());
            }
            break;
            case TensorKind::Float16:
            {
                return CreateTensor<TensorKind::Float16>(args, elementStrings, tensorDescriptor.Shape());
            }
            break;
            case TensorKind::Double:
            {
                return CreateTensor<TensorKind::Double>(args, elementStrings, tensorDescriptor.Shape());
            }
            break;
            case TensorKind::Int8:
            {
                return CreateTensor<TensorKind::Int8>(args, elementStrings, tensorDescriptor.Shape());
            }
            break;
            case TensorKind::UInt8:
            {
                return CreateTensor<TensorKind::UInt8>(args, elementStrings, tensorDescriptor.Shape());
            }
            break;
            case TensorKind::Int16:
            {
                return CreateTensor<TensorKind::Int16>(args, elementStrings, tensorDescriptor.Shape());
            }
            break;
            case TensorKind::UInt16:
            {
                return CreateTensor<TensorKind::UInt16>(args, elementStrings, tensorDescriptor.Shape());
            }
            break;
            case TensorKind::Int32:
            {
                return CreateTensor<TensorKind::Int32>(args, elementStrings, tensorDescriptor.Shape());
            }
            break;
            case TensorKind::UInt32:
            {
                return CreateTensor<TensorKind::UInt32>(args, elementStrings, tensorDescriptor.Shape());
            }
            break;
            case TensorKind::Int64:
            {
                return CreateTensor<TensorKind::Int64>(args, elementStrings, tensorDescriptor.Shape());
            }
            break;
            case TensorKind::UInt64:
            {
                return CreateTensor<TensorKind::UInt64>(args, elementStrings, tensorDescriptor.Shape());
            }
            break;
        }

        std::cout << "BindingUtilities: TensorKind has not been implemented." << std::endl;
        throw hresult_not_implemented();
    }

    ImageFeatureValue CreateBindableImage(const ILearningModelFeatureDescriptor& featureDescriptor,
                                          const std::wstring& imagePath, InputBindingType inputBindingType,
                                          InputDataType inputDataType, const IDirect3DDevice winrtDevice,
                                          const CommandLineArgs& args, uint32_t iterationNum)
    {
        auto softwareBitmap =
            imagePath.empty() ? GenerateGarbageImage(featureDescriptor, inputDataType)
                              : LoadImageFile(featureDescriptor, inputDataType, imagePath.c_str(), args, iterationNum);
        auto videoFrame = CreateVideoFrame(softwareBitmap, inputBindingType, inputDataType, winrtDevice);
        return ImageFeatureValue::CreateFromVideoFrame(videoFrame);
    }

    template <typename K, typename V>
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

    void PrintOrSaveEvaluationResults(const LearningModel& model, const CommandLineArgs& args,
                                      const IMapView<hstring, winrt::Windows::Foundation::IInspectable>& results,
                                      OutputHelper& output, int iterationNum)
    {
        for (auto&& desc : model.OutputFeatures())
        {
            if (desc.Kind() == LearningModelFeatureKind::Tensor)
            {
                std::string name = to_string(desc.Name());
                if (args.IsSaveTensor() && args.SaveTensorMode() == "First" && iterationNum > 0)
                {
                    return;
                }
                if (args.IsSaveTensor())
                {
                    output.SetDefaultCSVIterationResult(iterationNum, args, name);
                }
                void* tensor;
                uint32_t uCapacity;
                com_ptr<ITensorNative> itn = results.Lookup(desc.Name()).as<ITensorNative>();
                HRESULT(itn->GetBuffer(reinterpret_cast<BYTE**>(&tensor), &uCapacity));
                int size = 0;
                unsigned int topK = args.TopK();
                std::vector<std::pair<float, int>> maxKValues;
                std::ofstream fout;
                if (args.IsSaveTensor())
                {
                    fout.open(output.getCsvFileNamePerIterationResult(), std::ios_base::app);
                    fout << "Index"
                         << ","
                         << "Value" << std::endl;
                }
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
                        output.ProcessTensorResult<HALF>(args, tensor, uCapacity, maxKValues, fout, topK);
                    }
                    break;
                    case TensorKind::Float:
                    {
                        output.ProcessTensorResult<float>(args, tensor, uCapacity, maxKValues, fout, topK);
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
                if (args.IsSaveTensor())
                {
                    fout.close();
                    for (auto& pair : maxKValues)
                    {
                        auto maxValue = pair.first;
                        auto maxIndex = pair.second;
                        std::string iterationResult =
                            "Index: " + std::to_string(maxIndex) + "; Value: " + std::to_string(maxValue);
                        output.SaveResult(iterationNum, iterationResult,
                                          static_cast<int>(hash_data(tensor, uCapacity)));
                    }
                }
                if (!args.IsGarbageInput() && iterationNum == 0)
                {
                    std::cout << "Outputting top " << args.TopK() << " values" << std::endl;
                    std::cout << "Feature Name: " << name << std::endl;
                    for (auto& pair : maxKValues)
                    {
                        auto maxValue = pair.first;
                        auto maxIndex = pair.second;
                        std::wcout << " index: " << maxIndex << ", value: " << maxValue << std::endl;
                    }
                }
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
}; // namespace BindingUtilities
