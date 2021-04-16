#include "d3dx12.h"
#include <random>
#include <time.h>
#ifdef USE_WINML_NUGET
#include "Microsoft.AI.Machinelearning.Native.h"
#else
#include "Windows.AI.Machinelearning.Native.h"
#endif
#include "MemoryBuffer.h"
#include "TypeHelper.h"
#include "CommandLineArgs.h"
#include "OutputHelper.h"
#include "BindingUtilities.h"
using namespace winrt::Windows::Media;
using namespace winrt::Windows::Storage;
using namespace winrt::Windows::Storage::Streams;
#ifdef USE_WINML_NUGET
using namespace winrt::Microsoft::AI::MachineLearning;
#else
using namespace winrt::Windows::AI::MachineLearning;
#endif
using namespace winrt::Windows::Foundation::Collections;
using namespace winrt::Windows::Graphics::DirectX;
using namespace winrt::Windows::Graphics::Imaging;
using namespace winrt::Windows::Graphics::DirectX::Direct3D11;
using namespace DirectX::PackedVector;

inline size_t hash_data(void const* ptr, size_t const bytes) noexcept
{
#ifdef _WIN64
    constexpr size_t fnv_offset_basis = 14695981039346656037ULL;
    constexpr size_t fnv_prime = 1099511628211ULL;
#else
    constexpr size_t fnv_offset_basis = 2166136261U;
    constexpr size_t fnv_prime = 16777619U;
#endif
    size_t result = fnv_offset_basis;
    uint8_t const* const buffer = static_cast<uint8_t const*>(ptr);

    for (size_t next = 0; next < bytes; ++next)
    {
        result ^= buffer[next];
        result *= fnv_prime;
    }

    return result;
}

template <TensorKind T> struct TensorKindToPointerType
{
    static_assert(true, "No TensorKind mapped for given type!");
};
template <> struct TensorKindToPointerType<TensorKind::UInt8>
{
    typedef uint8_t Type;
};
template <> struct TensorKindToPointerType<TensorKind::Int8>
{
    typedef int8_t Type;
};
template <> struct TensorKindToPointerType<TensorKind::UInt16>
{
    typedef uint16_t Type;
};
template <> struct TensorKindToPointerType<TensorKind::Int16>
{
    typedef int16_t Type;
};
template <> struct TensorKindToPointerType<TensorKind::UInt32>
{
    typedef uint32_t Type;
};
template <> struct TensorKindToPointerType<TensorKind::Int32>
{
    typedef int32_t Type;
};
template <> struct TensorKindToPointerType<TensorKind::UInt64>
{
    typedef uint64_t Type;
};
template <> struct TensorKindToPointerType<TensorKind::Int64>
{
    typedef int64_t Type;
};
template <> struct TensorKindToPointerType<TensorKind::Boolean>
{
    typedef boolean Type;
};
template <> struct TensorKindToPointerType<TensorKind::Double>
{
    typedef double Type;
};
template <> struct TensorKindToPointerType<TensorKind::Float>
{
    typedef float Type;
};
template <> struct TensorKindToPointerType<TensorKind::Float16>
{
    typedef HALF Type;
};
template <> struct TensorKindToPointerType<TensorKind::String>
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

template <TensorKind T, typename ToType, typename FromType> ToType ConvertToPointerType(FromType value)
{
    static_assert(true, "No TensorKind mapped for given type!");
    return 0;
};
template <> uint8_t ConvertToPointerType<TensorKind::UInt8>(float value) { return static_cast<uint8_t>(value); };
template <> int8_t ConvertToPointerType<TensorKind::Int8>(float value) { return static_cast<int8_t>(value); };
template <> uint16_t ConvertToPointerType<TensorKind::UInt16>(float value) { return static_cast<uint16_t>(value); };
template <> int16_t ConvertToPointerType<TensorKind::Int16>(float value) { return static_cast<int16_t>(value); };
template <> uint32_t ConvertToPointerType<TensorKind::UInt32>(float value) { return static_cast<uint32_t>(value); };
template <> int32_t ConvertToPointerType<TensorKind::Int32>(float value) { return static_cast<int32_t>(value); };
template <> uint64_t ConvertToPointerType<TensorKind::UInt64>(float value) { return static_cast<uint64_t>(value); };
template <> int64_t ConvertToPointerType<TensorKind::Int64>(float value) { return static_cast<int64_t>(value); };
template <> boolean ConvertToPointerType<TensorKind::Boolean>(float value) { return static_cast<boolean>(value); };
template <> double ConvertToPointerType<TensorKind::Double>(double value) { return static_cast<double>(value); };
template <> float ConvertToPointerType<TensorKind::Float>(float value) { return static_cast<float>(value); };
template <> HALF ConvertToPointerType<TensorKind::Float16>(float value) { return XMConvertFloatToHalf(value); };
template <> winrt::hstring ConvertToPointerType<TensorKind::String>(winrt::hstring value)
{
    return static_cast<winrt::hstring>(value);
};

ColorManagementMode GetColorManagementMode(const LearningModel& model)

{
    // Get model color space gamma
    hstring gammaSpace = L"";
    try
    {
        gammaSpace = model.Metadata().Lookup(L"Image.ColorSpaceGamma");
    }
    catch (...)
    {
        printf("    Model does not have color space gamma information. Will color manage to sRGB by default...\n");
    }
    if (gammaSpace == L"" || _wcsicmp(gammaSpace.c_str(), L"SRGB") == 0)
    {
        return ColorManagementMode::ColorManageToSRgb;
    }
    // Due diligence should be done to make sure that the input image is within the model's colorspace. There are
    // multiple non-sRGB color spaces.
    printf("    Model metadata indicates that color gamma space is : %ws. Will not manage color space to sRGB...\n",
           gammaSpace.c_str());
    return ColorManagementMode::DoNotColorManage;
}

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
    static std::independent_bits_engine<std::default_random_engine, CHAR_BIT, unsigned int> randomBitsEngineChar;

    SoftwareBitmap GenerateGarbageImage(const ILearningModelFeatureDescriptor& modelFeatureDescriptor,
                                        InputDataType inputDataType)
    {
        assert(inputDataType != InputDataType::Tensor);
        uint64_t width = 0;
        uint64_t height = 0;
        GetHeightAndWidthFromLearningModelFeatureDescriptor(modelFeatureDescriptor, width, height);

        // We have to create RGBA8 or BGRA8 images, so we need 4 channels
        uint32_t totalByteSize = static_cast<uint32_t>(width) * static_cast<uint32_t>(height) * 4;

        // Generate values for the image based on a seed
        std::vector<uint8_t> data(totalByteSize);
        randomBitsEngineChar.seed(seed++);
        std::generate(data.begin(), data.end(), randomBitsEngineChar);

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
                                 const InputDataType inputDataType, const hstring& filePath,
                                 const CommandLineArgs& args, uint32_t iterationNum,
                                 ColorManagementMode colorManagementMode)
    {
        // We assume NCHW and NCDHW
        uint64_t width = 0;
        uint64_t height = 0;
        GetHeightAndWidthFromLearningModelFeatureDescriptor(modelFeatureDescriptor, width, height);
        IRandomAccessStream stream;
        BitmapDecoder decoder = NULL;
        try
        {
            // open the file
            StorageFile file = StorageFile::GetFileFromPathAsync(filePath).get();
            // get a stream on it
            stream = file.OpenAsync(FileAccessMode::Read).get();
            // Create the decoder from the stream
            decoder = BitmapDecoder::CreateAsync(stream).get();
        }
        catch (hresult_error hr)
        {
            printf("    Failed to load the image file, make sure you are using fully qualified paths\r\n");
            printf("    %ws\n", hr.message().c_str());
            exit(hr.code());
        }
        BitmapPixelFormat format = inputDataType == InputDataType::Tensor
                                       ? decoder.BitmapPixelFormat()
                                       : TypeHelper::GetBitmapPixelFormat(inputDataType);
        try
        {
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
                    .GetSoftwareBitmapAsync(format, decoder.BitmapAlphaMode(), transform,
                                            ExifOrientationMode::RespectExifOrientation, colorManagementMode)
                    .get();
            }
            else
            {
                // get the bitmap
                return decoder
                    .GetSoftwareBitmapAsync(format, decoder.BitmapAlphaMode(), BitmapTransform(),
                                            ExifOrientationMode::RespectExifOrientation, colorManagementMode)
                    .get();
            }
        }
        catch (hresult_error hr)
        {
            printf("    Failed to create SoftwareBitmap! Please make sure that input image is within the model's "
                   "colorspace.\n");
            printf("    %ws\n", hr.message().c_str());
            exit(hr.code());
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

    struct InputBufferDesc
    {
        uint8_t* elements;
        uint32_t totalSizeInBytes;
        uint32_t numChannelsPerElement;
        uint32_t elementStrideInBytes;
        bool isPlanar;
        TensorKind channelFormat;
        BitmapPixelFormat elementFormat;

        InputBufferDesc()
            : elements(nullptr), totalSizeInBytes(0), numChannelsPerElement(0), elementStrideInBytes(0), isPlanar(0),
              channelFormat(TensorKind::Undefined), elementFormat(BitmapPixelFormat::Unknown)
        {
        }
    };

    void ReadCSVIntoBuffer(const std::wstring& csvFilePath, InputBufferDesc& inputBufferDesc)
    {
        std::ifstream fileStream;
        fileStream.open(csvFilePath);
        if (!fileStream.is_open())
        {
            ThrowFailure(L"BindingUtilities: could not open data file.");
        }

        uint32_t pos = 0;
        std::string line;
        float_t* pData = (float_t*)inputBufferDesc.elements;
        uint32_t expectedPos = (inputBufferDesc.totalSizeInBytes * inputBufferDesc.numChannelsPerElement) /
                               inputBufferDesc.elementStrideInBytes;
        while (std::getline(fileStream, line, ','))
        {
            if (pos > expectedPos)
                throw hresult_invalid_argument(L"Too many elements in CSV file to fit in input of what model expects!");
            *pData = std::stof(line);
            ++pData;

            ++pos;
            if (pos >= inputBufferDesc.totalSizeInBytes)
                break;
        }

        // Check to see if csv didn't fill in entire buffer and throw or fill with zeros?
        if (pos != expectedPos)
        {
            throw hresult_invalid_argument(L"CSV input size/shape is different from what model expects!");
        }
    }

    // Roll the array correctly for the tensor
    template <TensorKind TKind, typename InputType>
    void CopyTensorFromBuffer(void* actualData, uint32_t tensorHeight, uint32_t tensorWidth,
                              const InputBufferDesc& inputBufferDesc, float scale, const std::vector<float>& means,
                              const std::vector<float>& stddevs)
    {
        using WriteType = typename TensorKindToPointerType<TKind>::Type;

        WriteType* pDataOut = static_cast<WriteType*>(actualData);
        InputType* pDataIn = (InputType*)inputBufferDesc.elements;
        uint32_t elementOffsetMultiplier = inputBufferDesc.isPlanar ? inputBufferDesc.numChannelsPerElement : 1;
        uint32_t channelOffsetMultiplier = inputBufferDesc.isPlanar ? 1 : tensorHeight * tensorWidth;
        for (uint32_t element = 0; element < tensorHeight * tensorWidth; ++element)
        {
            for (uint32_t channel = 0; channel < inputBufferDesc.numChannelsPerElement; ++channel)
            {
                pDataOut[element * elementOffsetMultiplier + channel * channelOffsetMultiplier] =
                    ConvertToPointerType<TKind, WriteType>(((pDataIn[channel] / scale) - means[channel]) /
                                                           stddevs[channel]);
            }
            pDataIn += inputBufferDesc.elementStrideInBytes / sizeof(InputType);
        }
    }

    template <TensorKind TKind, typename WriteType>
    static void GenerateRandomData(WriteType* data, uint32_t sizeInBytes, uint32_t maxValue)
    {
        static std::independent_bits_engine<std::default_random_engine, sizeof(uint32_t) * 8, uint32_t>
            randomBitsEngine;
        randomBitsEngine.seed(seed++);

        WriteType* begin = data;
        WriteType* end = reinterpret_cast<WriteType*>(reinterpret_cast<BYTE*>(data) + sizeInBytes);
        while (begin <= end)
        {
            *begin = maxValue * static_cast<float>(randomBitsEngine()) / (randomBitsEngine.max)();
            ++begin;
        }
    }

    template <TensorKind TKind>
    static ITensor CreateTensor(const CommandLineArgs& args, const std::vector<int64_t>& tensorShape,
                                const InputBindingType inputBindingType, const InputBufferDesc& inputBufferDesc)
    {
        using TensorValue = typename TensorKindToValue<TKind>::Type;
        using WriteType = typename TensorKindToPointerType<TKind>::Type;

        // Map the incoming Tensor as a TensorNative to get the actual data buffer.
        auto tensorValue = TensorValue::Create(tensorShape);

        com_ptr<ITensorNative> spTensorValueNative;
        tensorValue.as(spTensorValueNative);

        WriteType* actualData;
        uint32_t actualSizeInBytes;
        THROW_IF_FAILED(spTensorValueNative->GetBuffer(reinterpret_cast<BYTE**>(&actualData), &actualSizeInBytes));

        if (args.IsCSVInput() || args.IsImageInput())
        {
            // Assumes NCHW
            uint32_t channels = static_cast<uint32_t>(tensorShape[1]);
            uint32_t tensorHeight = static_cast<uint32_t>(tensorShape[2]);
            uint32_t tensorWidth = static_cast<uint32_t>(tensorShape[3]);

            // Check to make sure the sizes are right
            uint32_t inputElementCount = inputBufferDesc.totalSizeInBytes / inputBufferDesc.elementStrideInBytes;
            uint32_t outputElementCount = actualSizeInBytes / (channels * sizeof(WriteType));
            if (inputElementCount != outputElementCount)
            {
                throw hresult_invalid_argument(L"Input size / shape is different from what the model expects");
            }

            float scale;
            std::vector<float> means = {};
            std::vector<float> stddevs = {};

            const auto& tensorizeArgs = args.TensorizeArgs();
            const auto& normalizeParams = tensorizeArgs.Normalize;
            switch (tensorizeArgs.Func)
            {
                case TensorizeFuncs::Identity:
                    scale = 1.0f;
                    means.resize(channels, 0.0f);
                    stddevs.resize(channels, 1.0f);
                    break;
                case TensorizeFuncs::Normalize:
                    switch (inputBufferDesc.elementFormat)
                    {
                        case BitmapPixelFormat::Gray8:
                        case BitmapPixelFormat::Gray16:
                        case BitmapPixelFormat::Rgba8:
                        case BitmapPixelFormat::Rgba16:
                            scale = normalizeParams.Scale;
                            means.resize(channels);
                            stddevs.resize(channels);
                            for (uint32_t i = 0; i < channels; ++i)
                            {
                                means[i] = normalizeParams.Means[i];
                                stddevs[i] = normalizeParams.StdDevs[i];
                            }
                            break;
                        case BitmapPixelFormat::Bgra8:
                            scale = normalizeParams.Scale;
                            means.resize(channels);
                            stddevs.resize(channels);
                            for (uint32_t i = 0; i < channels; ++i)
                            {
                                means[channels - 1 - i] = normalizeParams.Means[i];
                                stddevs[channels - 1 - i] = normalizeParams.StdDevs[i];
                            }
                            break;

                        default:
                            throw hresult_invalid_argument(
                                L"CreateTensor<TKind>: Unhandled SoftwareBitmap pixel format");
                    }
                    break;
                default:
                    throw hresult_invalid_argument(L"CreateTensor<TKind>: Unknown Tensorize Function");
            }

            switch (inputBufferDesc.channelFormat)
            {
                case TensorKind::UInt8:
                    CopyTensorFromBuffer<TKind, uint8_t>(actualData, tensorHeight, tensorWidth, inputBufferDesc, scale,
                                                         means, stddevs);
                    break;
                case TensorKind::Float:
                    CopyTensorFromBuffer<TKind, float>(actualData, tensorHeight, tensorWidth, inputBufferDesc, scale,
                                                       means, stddevs);
                    break;
                default:
                    throw hresult_not_implemented(L"Creating Tensors for Input Images with unhandled channel format!");
            }
        }
        // Garbage Data
        else if (args.IsGarbageDataRange())
        {
            GenerateRandomData<TKind>(actualData, actualSizeInBytes, args.GarbageDataMaxValue());
        }

        if (inputBindingType == InputBindingType::CPU)
        {
            return tensorValue;
        }
        else // GPU Tensor
        {
            com_ptr<ID3D12Resource> pGPUResource = nullptr;
            try
            {
                // create the d3d device.
                com_ptr<ID3D12Device> pD3D12Device = nullptr;
                D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL::D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device),
                                  reinterpret_cast<void**>(&pD3D12Device));

                auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
                auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(actualSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
                pD3D12Device->CreateCommittedResource(
                    &heapProperties, D3D12_HEAP_FLAG_NONE,
                    &resourceDesc,
                    D3D12_RESOURCE_STATE_COMMON, nullptr, __uuidof(ID3D12Resource), pGPUResource.put_void());

                if (!args.IsGarbageInput())
                {
                    com_ptr<ID3D12Resource> imageUploadHeap;
                    // Create the GPU upload buffer.
                    auto heapProperties2 = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
                    auto resourceDesc2 = CD3DX12_RESOURCE_DESC::Buffer(actualSizeInBytes);
                    pD3D12Device->CreateCommittedResource(
                        &heapProperties2, D3D12_HEAP_FLAG_NONE,
                        &resourceDesc2, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                        __uuidof(ID3D12Resource), imageUploadHeap.put_void());

                    // create the command queue.
                    com_ptr<ID3D12CommandQueue> dxQueue = nullptr;
                    D3D12_COMMAND_QUEUE_DESC commandQueueDesc = {};
                    commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
                    pD3D12Device->CreateCommandQueue(&commandQueueDesc, __uuidof(ID3D12CommandQueue),
                                                     reinterpret_cast<void**>(&dxQueue));
                    com_ptr<ILearningModelDeviceFactoryNative> devicefactory =
                        get_activation_factory<LearningModelDevice, ILearningModelDeviceFactoryNative>();
                    com_ptr<::IUnknown> spUnk;
                    devicefactory->CreateFromD3D12CommandQueue(dxQueue.get(), spUnk.put());

                    // Create ID3D12GraphicsCommandList and Allocator
                    D3D12_COMMAND_LIST_TYPE queuetype = dxQueue->GetDesc().Type;
                    com_ptr<ID3D12CommandAllocator> alloctor;
                    com_ptr<ID3D12GraphicsCommandList> cmdList;
                    pD3D12Device->CreateCommandAllocator(queuetype, winrt::guid_of<ID3D12CommandAllocator>(),
                                                         alloctor.put_void());
                    pD3D12Device->CreateCommandList(0, queuetype, alloctor.get(), nullptr,
                                                    winrt::guid_of<ID3D12CommandList>(), cmdList.put_void());

                    // Copy from Cpu to GPU
                    D3D12_SUBRESOURCE_DATA CPUData = {};
                    CPUData.pData = actualData;
                    CPUData.RowPitch = actualSizeInBytes;
                    CPUData.SlicePitch = actualSizeInBytes;
                    UpdateSubresources(cmdList.get(), pGPUResource.get(), imageUploadHeap.get(), 0, 0, 1, &CPUData);

                    // Close the command list and execute it to begin the initial GPU setup.
                    cmdList->Close();
                    ID3D12CommandList* ppCommandLists[] = { cmdList.get() };
                    dxQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

                    // Create Event
                    HANDLE directEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

                    // Create Fence
                    ::Microsoft::WRL::ComPtr<ID3D12Fence> spDirectFence = nullptr;
                    THROW_IF_FAILED(pD3D12Device->CreateFence(0, D3D12_FENCE_FLAG_NONE,
                                                              IID_PPV_ARGS(spDirectFence.ReleaseAndGetAddressOf())));
                    // Adds fence to queue
                    THROW_IF_FAILED(dxQueue->Signal(spDirectFence.Get(), 1));
                    THROW_IF_FAILED(spDirectFence->SetEventOnCompletion(1, directEvent));

                    // Wait for signal
                    DWORD retVal = WaitForSingleObject(directEvent, INFINITE);
                    if (retVal != WAIT_OBJECT_0)
                    {
                        THROW_IF_FAILED(E_UNEXPECTED);
                    }
                }
            }
            catch (...)
            {
                std::cout << "Couldn't create and copy CPU tensor resource to GPU resource" << std::endl;
                throw;
            }
            com_ptr<ITensorStaticsNative> tensorfactory = get_activation_factory<TensorValue, ITensorStaticsNative>();
            com_ptr<::IUnknown> spUnkTensor;
            tensorfactory->CreateFromD3D12Resource(pGPUResource.get(), const_cast<int64_t*>(tensorShape.data()),
                                                   static_cast<int>(tensorShape.size()), spUnkTensor.put());
            TensorValue returnTensor(nullptr);
            spUnkTensor.try_as(returnTensor);
            return returnTensor;
        }
    }

    // Process the descriptor to gather and normalize the shape
    void ProcessDescriptor(const ILearningModelFeatureDescriptor& description, std::vector<int64_t>& shape,
                           TensorKind& tensorKind, InputBufferDesc& inputBufferDesc)
    {
        // Try Image Feature Descriptor
        auto imageFeatureDescriptor = description.try_as<ImageFeatureDescriptor>();
        if (imageFeatureDescriptor)
        {
            int64_t channels;
            inputBufferDesc.elementFormat = imageFeatureDescriptor.BitmapPixelFormat();
            switch (inputBufferDesc.elementFormat)
            {
                case BitmapPixelFormat::Gray8:
                case BitmapPixelFormat::Gray16:
                    channels = 1;
                    break;
                case BitmapPixelFormat::Bgra8:
                case BitmapPixelFormat::Rgba16:
                case BitmapPixelFormat::Rgba8:
                    channels = 3;
                    break;
                default:
                    throw hresult_not_implemented(L"BitmapPixel format not yet handled by WinMLRunner.");
            }

            tensorKind = TensorKind::Float;
            shape.push_back(1);
            shape.push_back(channels);
            shape.push_back(static_cast<int64_t>(imageFeatureDescriptor.Height()));
            shape.push_back(static_cast<int64_t>(imageFeatureDescriptor.Width()));
            return;
        }

        auto tensorDescriptor = description.try_as<TensorFeatureDescriptor>();
        if (tensorDescriptor)
        {
            IVectorView<int64_t> tensorShape = tensorDescriptor.Shape();
            for (uint32_t dim = 0; dim < tensorShape.Size(); dim++)
            {
                int64_t dimSize = tensorShape.GetAt(dim);
                if (dimSize > 0) // If the dimension is greater than 0, then it is known.
                {
                    shape.push_back(dimSize);
                }
                else // otherwise, make sure that the dimension is -1, representing free dimension. If not, then it's an
                     // invalid model.
                {
                    if (dimSize == -1)
                    {
                        shape.push_back(1);
                    }
                    else
                    {
                        throw hresult_invalid_argument(L"Failed to create a tensor with an unknown dimension of: " +
                                                       dimSize);
                    }
                }
            }

            tensorKind = tensorDescriptor.TensorKind();
            return;
        }

        throw hresult_invalid_argument(L"ProcessDescriptor: Unknown desription type!");
    } // namespace BindingUtilities

    // Binds tensor floats, ints, doubles from CSV data.
    ITensor CreateBindableTensor(const ILearningModelFeatureDescriptor& description, const std::wstring& imagePath,
                                 const InputBindingType inputBindingType, const InputDataType inputDataType,
                                 const CommandLineArgs& args, uint32_t iterationNum,
                                 ColorManagementMode colorManagementMode)
    {
        InputBufferDesc inputBufferDesc = {};

        std::vector<int64_t> shape = {};
        TensorKind tensorKind = TensorKind::Undefined;
        ProcessDescriptor(description, shape, tensorKind, inputBufferDesc);

        SoftwareBitmap softwareBitmap(nullptr);
        if (args.IsCSVInput())
        {
            inputBufferDesc.channelFormat = TensorKind::Float;
            inputBufferDesc.isPlanar = true;

            // Assumes shape is in the format of 'NCHW'
            inputBufferDesc.numChannelsPerElement = static_cast<uint32_t>(shape[1]);

            // Assumes no gaps in the input csv file
            inputBufferDesc.elementStrideInBytes = inputBufferDesc.numChannelsPerElement * sizeof(float_t);

            inputBufferDesc.totalSizeInBytes = sizeof(float_t);
            for (uint32_t i = 0; i < shape.size(); ++i)
                inputBufferDesc.totalSizeInBytes *= static_cast<uint32_t>(shape[i]);

            inputBufferDesc.elements = new uint8_t[inputBufferDesc.totalSizeInBytes];

            ReadCSVIntoBuffer(args.CsvPath(), inputBufferDesc);
        }
        else if (args.IsImageInput())
        {
            softwareBitmap =
                LoadImageFile(description, inputDataType, imagePath.c_str(), args, iterationNum, colorManagementMode);

            // Get Pointers to the SoftwareBitmap data buffers
            const BitmapBuffer sbBitmapBuffer(softwareBitmap.LockBuffer(BitmapBufferAccessMode::Read));
            winrt::Windows::Foundation::IMemoryBufferReference sbReference = sbBitmapBuffer.CreateReference();
            auto sbByteAccess = sbReference.as<::Windows::Foundation::IMemoryBufferByteAccess>();
            winrt::check_hresult(sbByteAccess->GetBuffer(&inputBufferDesc.elements, &inputBufferDesc.totalSizeInBytes));

            inputBufferDesc.isPlanar = false;
            inputBufferDesc.elementFormat = softwareBitmap.BitmapPixelFormat();
            switch (inputBufferDesc.elementFormat)
            {
                case BitmapPixelFormat::Gray8:
                    inputBufferDesc.channelFormat = TensorKind::UInt8;
                    inputBufferDesc.numChannelsPerElement = 1;
                    inputBufferDesc.elementStrideInBytes = sizeof(uint8_t);
                    break;
                case BitmapPixelFormat::Gray16:
                    inputBufferDesc.channelFormat = TensorKind::UInt16;
                    inputBufferDesc.numChannelsPerElement = 1;
                    inputBufferDesc.elementStrideInBytes = sizeof(uint16_t);
                    break;
                case BitmapPixelFormat::Bgra8:
                    inputBufferDesc.channelFormat = TensorKind::UInt8;
                    inputBufferDesc.numChannelsPerElement = 3;
                    inputBufferDesc.elementStrideInBytes = 4 * sizeof(uint8_t);
                    break;
                case BitmapPixelFormat::Rgba8:
                    inputBufferDesc.channelFormat = TensorKind::UInt8;
                    inputBufferDesc.numChannelsPerElement = 3;
                    inputBufferDesc.elementStrideInBytes = 4 * sizeof(uint8_t);
                    break;
                case BitmapPixelFormat::Rgba16:
                    inputBufferDesc.channelFormat = TensorKind::UInt16;
                    inputBufferDesc.numChannelsPerElement = 3;
                    inputBufferDesc.elementStrideInBytes = 4 * sizeof(uint16_t);
                    break;
                default:
                    throw hresult_invalid_argument(L"Unknown BitmapPixelFormat in input image.");
            }
        }

        switch (tensorKind)
        {
            case TensorKind::Undefined:
            {
                std::cout << "BindingUtilities: TensorKind is undefined." << std::endl;
                throw hresult_invalid_argument();
            }
            case TensorKind::Float:
            {
                return CreateTensor<TensorKind::Float>(args, shape, inputBindingType, inputBufferDesc);
            }
            break;
            case TensorKind::Float16:
            {
                return CreateTensor<TensorKind::Float16>(args, shape, inputBindingType, inputBufferDesc);
            }
            break;
            case TensorKind::Double:
            {
                return CreateTensor<TensorKind::Double>(args, shape, inputBindingType, inputBufferDesc);
            }
            break;
            case TensorKind::Int8:
            {
                return CreateTensor<TensorKind::Int8>(args, shape, inputBindingType, inputBufferDesc);
            }
            break;
            case TensorKind::UInt8:
            {
                return CreateTensor<TensorKind::UInt8>(args, shape, inputBindingType, inputBufferDesc);
            }
            break;
            case TensorKind::Int16:
            {
                return CreateTensor<TensorKind::Int16>(args, shape, inputBindingType, inputBufferDesc);
            }
            break;
            case TensorKind::UInt16:
            {
                return CreateTensor<TensorKind::UInt16>(args, shape, inputBindingType, inputBufferDesc);
            }
            break;
            case TensorKind::Int32:
            {
                return CreateTensor<TensorKind::Int32>(args, shape, inputBindingType, inputBufferDesc);
            }
            break;
            case TensorKind::UInt32:
            {
                return CreateTensor<TensorKind::UInt32>(args, shape, inputBindingType, inputBufferDesc);
            }
            break;
            case TensorKind::Int64:
            {
                return CreateTensor<TensorKind::Int64>(args, shape, inputBindingType, inputBufferDesc);
            }
            break;
            case TensorKind::UInt64:
            {
                return CreateTensor<TensorKind::UInt64>(args, shape, inputBindingType, inputBufferDesc);
            }
            break;
        }
        std::cout << "BindingUtilities: TensorKind has not been implemented." << std::endl;
        throw hresult_not_implemented();
    }

    ImageFeatureValue CreateBindableImage(const ILearningModelFeatureDescriptor& featureDescriptor,
                                          const std::wstring& imagePath, InputBindingType inputBindingType,
                                          InputDataType inputDataType, const IDirect3DDevice winrtDevice,
                                          const CommandLineArgs& args, uint32_t iterationNum,
                                          ColorManagementMode colorManagementMode)
    {
        auto softwareBitmap = imagePath.empty() ? GenerateGarbageImage(featureDescriptor, inputDataType)
                                                : LoadImageFile(featureDescriptor, inputDataType, imagePath.c_str(),
                                                                args, iterationNum, colorManagementMode);
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
                std::wstring name(desc.Name());
                if (args.IsSaveTensor() && args.SaveTensorMode() == L"First" && iterationNum > 0)
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
                    fout.open(output.GetCsvFileNamePerIterationResult(), std::ios_base::app);
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
                    std::wcout << L"Outputting top " << args.TopK() << L" values" << std::endl;
                    std::wcout << L"Feature Name: " << name << std::endl;
                    for (auto& pair : maxKValues)
                    {
                        auto maxValue = pair.first;
                        auto maxIndex = pair.second;
                        std::wcout << L" index: " << maxIndex << L", value: " << maxValue << std::endl;
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
