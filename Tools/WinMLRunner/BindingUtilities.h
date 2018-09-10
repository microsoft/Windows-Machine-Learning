#pragma once
#include "Common.h"
#include "ModelBinding.h"
using namespace winrt::Windows::Graphics::Imaging;
using namespace Windows::Media;
using namespace winrt::Windows::Foundation::Collections;
using namespace Windows::Storage;
using namespace winrt::Windows::AI::MachineLearning;

namespace BindingUtilities
{
    bool BindTensorsFromGarbageData(LearningModelBinding context, LearningModel model)
    {
        for (auto&& description : model.InputFeatures())
        {
            if (description == nullptr)
            {
                std::cout << "BindingUtilities: Learning model has no binding description." << std::endl;
                return false;
            }

            hstring name = description.Name();
            TensorFeatureDescriptor tensorDescriptor = nullptr;
            try
            {
                tensorDescriptor = description.as<TensorFeatureDescriptor>();
            }
            catch (...)
            {
                std::cout << "BindingUtilities: TensorKind is undefined." << std::endl;
                return false;
            }

            TensorKind tensorKind = tensorDescriptor.TensorKind();
            switch (tensorKind)
            {
                case TensorKind::Undefined:
                {
                    std::cout << "BindingUtilities: TensorKind is undefined." << std::endl;
                    return false;
                }
                case TensorKind::Float:
                {
                    ModelBinding<float> binding(description);
                    ITensor tensor = TensorFloat::CreateFromArray(binding.GetShapeBuffer(), binding.GetDataBuffer());
                    context.Bind(name, tensor);
                }
                break;
                case TensorKind::Float16:
                {
                    ModelBinding<float> binding(description);
                    ITensor tensor = TensorFloat16Bit::CreateFromArray(binding.GetShapeBuffer(), binding.GetDataBuffer());
                    context.Bind(name, tensor);
                }
                break;
                case TensorKind::Double:
                {
                    ModelBinding<double> binding(description);
                    ITensor tensor = TensorDouble::CreateFromArray(binding.GetShapeBuffer(), binding.GetDataBuffer());
                    context.Bind(name, tensor);
                }
                break;
                case TensorKind::Int8:
                {
                    ModelBinding<uint8_t> binding(description);
                    ITensor tensor = TensorInt8Bit::CreateFromArray(binding.GetShapeBuffer(), binding.GetDataBuffer());
                    context.Bind(name, tensor);
                }
                break;
                case TensorKind::UInt8:
                {
                    ModelBinding<uint8_t> binding(description);
                    ITensor tensor = TensorUInt8Bit::CreateFromArray(binding.GetShapeBuffer(), binding.GetDataBuffer());
                    context.Bind(name, tensor);
                }
                break;
                case TensorKind::Int16:
                {
                    ModelBinding<int16_t> binding(description);
                    ITensor tensor = TensorInt16Bit::CreateFromArray(binding.GetShapeBuffer(), binding.GetDataBuffer());
                    context.Bind(name, tensor);
                }
                break;
                case TensorKind::UInt16:
                {
                    ModelBinding<uint16_t> binding(description);
                    ITensor tensor = TensorUInt16Bit::CreateFromArray(binding.GetShapeBuffer(), binding.GetDataBuffer());
                    context.Bind(name, tensor);
                }
                break;
                case TensorKind::Int32:
                {
                    ModelBinding<int32_t> binding(description);
                    ITensor tensor = TensorInt32Bit::CreateFromArray(binding.GetShapeBuffer(), binding.GetDataBuffer());
                    context.Bind(name, tensor);
                }
                break;
                case TensorKind::UInt32:
                {
                    ModelBinding<uint32_t> binding(description);
                    ITensor tensor = TensorUInt32Bit::CreateFromArray(binding.GetShapeBuffer(), binding.GetDataBuffer());
                    context.Bind(name, tensor);
                }
                break;
                case TensorKind::Int64:
                {
                    ModelBinding<int64_t> binding(description);
                    ITensor tensor = TensorInt64Bit::CreateFromArray(binding.GetShapeBuffer(), binding.GetDataBuffer());
                    context.Bind(name, tensor);
                }
                break;
                case TensorKind::UInt64:
                {
                    ModelBinding<uint64_t> binding(description);
                    ITensor tensor = TensorUInt64Bit::CreateFromArray(binding.GetShapeBuffer(), binding.GetDataBuffer());
                    context.Bind(name, tensor);
                }
                break;
                case TensorKind::String:
                {
                    ModelBinding<hstring> binding(description);
                    ITensor tensor = TensorString::CreateFromArray(binding.GetShapeBuffer(), binding.GetDataBuffer());
                    context.Bind(name, tensor);
                }
                break;
                default:
                {
                    std::cout << "BindingUtilities: TensorKind binding has not been implemented." << std::endl;
                    return false;
                }
            }
        }
        return true;
    }
    
    VideoFrame LoadImageFile(hstring filePath)
    {
        try
        {
            // open the file
            StorageFile file = StorageFile::GetFileFromPathAsync(filePath).get();
            // get a stream on it
            auto stream = file.OpenAsync(FileAccessMode::Read).get();
            // Create the decoder from the stream
            BitmapDecoder decoder = BitmapDecoder::CreateAsync(stream).get();
            // get the bitmap
            SoftwareBitmap softwareBitmap = decoder.GetSoftwareBitmapAsync().get();
            // load a videoframe from it
            VideoFrame inputImage = VideoFrame::CreateWithSoftwareBitmap(softwareBitmap);
            // all done
            return inputImage;
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
        assert(binding.GetDataBufferSize() == elementStrings.size());
        T* data = binding.GetData();
        for (auto &elementString : elementStrings)
        {
            T value;
            std::stringstream(elementString) >> value;
            data = &value;
            data++;
        }
    }

    // Binds tensor floats, ints, doubles from CSV data.
    bool BindCSVDataToContext(LearningModelBinding context, LearningModel model, std::wstring csvFilePath)
    {
        std::ifstream fileStream;
        fileStream.open(csvFilePath);
        if (!fileStream.is_open())
        {
            ThrowFailure(L"BindingUtilities: could not open data file.");
        }
        for (auto&& description : model.InputFeatures())
        {
            if (description == nullptr)
            {

                std::cout << "BindingUtilities: Learning model has no binding description." << std::endl;
                return false;
            }

            hstring name = description.Name();
            TensorFeatureDescriptor tensorDescriptor = description.as<TensorFeatureDescriptor>();
            TensorKind tensorKind = tensorDescriptor.TensorKind();

            std::vector<std::string> elementStrings = ReadCsvLine(fileStream);
            switch (tensorKind)
            {
                case TensorKind::Undefined:
                {
                    std::cout << "BindingUtilities: TensorKind is undefined." << std::endl;
                    return false;
                }
                case TensorKind::Float:
                {
                    ModelBinding<float> binding(description);
                    WriteDataToBinding<float>(elementStrings, binding);
                    ITensor tensor = TensorFloat::CreateFromArray(binding.GetShapeBuffer(), binding.GetDataBuffer());
                    context.Bind(name, tensor);
                }
                break;
                case TensorKind::Float16:
                {
                    ModelBinding<float> binding(description);
                    WriteDataToBinding<float>(elementStrings, binding);
                    ITensor tensor = TensorFloat16Bit::CreateFromArray(binding.GetShapeBuffer(), binding.GetDataBuffer());
                    context.Bind(name, tensor);
                }
                break;
                case TensorKind::Double:
                {
                    ModelBinding<double> binding(description);
                    WriteDataToBinding<double>(elementStrings, binding);
                    ITensor tensor = TensorDouble::CreateFromArray(binding.GetShapeBuffer(), binding.GetDataBuffer());
                    context.Bind(name, tensor);
                }
                break;
                case TensorKind::Int8:
                {
                    ModelBinding<uint8_t> binding(description);
                    WriteDataToBinding<uint8_t>(elementStrings, binding);
                    ITensor tensor = TensorInt8Bit::CreateFromArray(binding.GetShapeBuffer(), binding.GetDataBuffer());
                    context.Bind(name, tensor);
                }
                break;
                case TensorKind::UInt8:
                {
                    ModelBinding<uint8_t> binding(description);
                    WriteDataToBinding<uint8_t>(elementStrings, binding);
                    ITensor tensor = TensorUInt8Bit::CreateFromArray(binding.GetShapeBuffer(), binding.GetDataBuffer());
                    context.Bind(name, tensor);
                }
                break;
                case TensorKind::Int16:
                {
                    ModelBinding<int16_t> binding(description);
                    WriteDataToBinding<int16_t>(elementStrings, binding);
                    ITensor tensor = TensorInt16Bit::CreateFromArray(binding.GetShapeBuffer(), binding.GetDataBuffer());
                    context.Bind(name, tensor);
                }
                break;
                case TensorKind::UInt16:
                {
                    ModelBinding<uint16_t> binding(description);
                    WriteDataToBinding<uint16_t>(elementStrings, binding);
                    ITensor tensor = TensorUInt16Bit::CreateFromArray(binding.GetShapeBuffer(), binding.GetDataBuffer());
                    context.Bind(name, tensor);
                }
                break;
                case TensorKind::Int32:
                {
                    ModelBinding<int32_t> binding(description);
                    WriteDataToBinding<int32_t>(elementStrings, binding);
                    ITensor tensor = TensorInt32Bit::CreateFromArray(binding.GetShapeBuffer(), binding.GetDataBuffer());
                    context.Bind(name, tensor);
                }
                break;
                case TensorKind::UInt32:
                {
                    ModelBinding<uint32_t> binding(description);
                    WriteDataToBinding<uint32_t>(elementStrings, binding);
                    ITensor tensor = TensorUInt32Bit::CreateFromArray(binding.GetShapeBuffer(), binding.GetDataBuffer());
                    context.Bind(name, tensor);
                }
                break;
                case TensorKind::Int64:
                {
                    ModelBinding<int64_t> binding(description);
                    WriteDataToBinding<int64_t>(elementStrings, binding);
                    ITensor tensor = TensorInt64Bit::CreateFromArray(binding.GetShapeBuffer(), binding.GetDataBuffer());
                    context.Bind(name, tensor);
                }
                break;
                case TensorKind::UInt64:
                {
                    ModelBinding<uint64_t> binding(description);
                    WriteDataToBinding<uint64_t>(elementStrings, binding);
                    ITensor tensor = TensorUInt64Bit::CreateFromArray(binding.GetShapeBuffer(), binding.GetDataBuffer());
                    context.Bind(name, tensor);
                }
                break;
                default:
                {
                    std::cout << "BindingUtilities: TensorKind has not been implemented." << std::endl;
                    return false;
                }
            }
            return true;
        }
    }

    bool BindImageToContext(LearningModelBinding context, LearningModel model, std::wstring imagePath)
    {
        context.Clear();
        for (auto&& description : model.InputFeatures())
        {
            hstring name = description.Name();
            auto Kind = description.Kind();
            auto videoFrame = LoadImageFile(imagePath.c_str());
            if (videoFrame == nullptr)
            {
                std::cout << "BindingUtilities: Cannot bind image to LearningModelBinding." << std::endl;
                std::cout << std::endl;
                return false;
            }
            try
            {
                auto featureValue = ImageFeatureValue::CreateFromVideoFrame(videoFrame);
                context.Bind(name, featureValue);
            }
            catch (const std::wstring &msg)
            {
                WriteErrorMsg(msg);
                std::cout << std::endl;
                return false;
            }
            catch (HRESULT hr)
            {
                std::cout << hr << std::endl;
                return false;
            }
            catch (hresult_error hr)
            {
                std::wcout << hr.message().c_str() << std::endl;
                return false;
            }
        }
        return true;
    }

    template< typename K, typename V>
    void OutputSequenceBinding(IMapView<hstring, Windows::Foundation::IInspectable> results, hstring name)
    {
        auto map = results.Lookup(name).as<IVectorView<IMap<int64_t, float>>>().GetAt(0);
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

    void PrintEvaluationResults(LearningModel model, CommandLineArgs args, IMapView<hstring, Windows::Foundation::IInspectable> results)
    {
        std::cout << "Outputting results.. " << std::endl;
        for (auto&& desc : model.OutputFeatures())
        {
            if (desc.Kind() == LearningModelFeatureKind::Tensor)
            {
                std::wstring name(desc.Name());
                std::wcout << "Feature Name: " << name <<std::endl;
                TensorFeatureDescriptor tensorDescriptor = desc.as<TensorFeatureDescriptor>();
                TensorKind tensorKind = tensorDescriptor.TensorKind();
                switch (tensorKind)
                {
                case TensorKind::String:
                {
                    auto resultVector = results.Lookup(desc.Name()).as<TensorString>().GetAsVectorView();
                    auto output = resultVector.GetAt(0).data();
                    std::wcout << " Result: " << output << std::endl;
                }
                break;
                case TensorKind::Float:
                {
                    auto resultVector = results.Lookup(desc.Name()).as<TensorFloat>().GetAsVectorView();
                    auto output = resultVector.GetAt(0);
                    std::wcout << " Result: " << output << std::endl;
                }
                break;
                case TensorKind::Int64:
                {
                    auto resultVector = results.Lookup(desc.Name()).as<TensorInt64Bit>().GetAsVectorView();
                    auto output = resultVector.GetAt(0);
                    std::wcout << " Result: " << output << std::endl;
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
            std::cout << std::endl;
        }
    }

    bool BindGarbageDataToContext(LearningModelBinding context, LearningModel model)
    {
        context.Clear();
        return BindTensorsFromGarbageData(context, model);
    }
 };
