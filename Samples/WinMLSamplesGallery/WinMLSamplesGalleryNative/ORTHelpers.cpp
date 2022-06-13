#include "pch.h"
#include "ORTHelpers.h"
#undef min

using namespace DirectX;

using TensorInt64Bit = winrt::Microsoft::AI::MachineLearning::TensorInt64Bit;
using TensorKind = winrt::Microsoft::AI::MachineLearning::TensorKind;
using LearningModelBuilder = winrt::Microsoft::AI::MachineLearning::Experimental::LearningModelBuilder;
using LearningModelOperator = winrt::Microsoft::AI::MachineLearning::Experimental::LearningModelOperator;

#define THROW_IF_FAILED(hr) {HRESULT localHr = (hr); if (FAILED(hr)) throw hr;}
#define RETURN_IF_FAILED(hr) {HRESULT localHr = (hr); if (FAILED(hr)) return hr;}
#define THROW_IF_NOT_OK(status) {auto localStatus = (status); if (localStatus) throw E_FAIL;}
#define RETURN_HR_IF_NOT_OK(status) {auto localStatus = (status); if (localStatus) return E_FAIL;}

template <typename T>
using BaseType =
std::remove_cv_t<
    std::remove_reference_t<
    std::remove_pointer_t<
    std::remove_all_extents_t<T>
    >
    >
>;

template<typename T>
using deleting_unique_ptr = std::unique_ptr<T, std::function<void(T*)>>;

template <typename C, typename T = BaseType<decltype(*std::declval<C>().data())>>
T GetElementCount(C const& range)
{
    return std::accumulate(range.begin(), range.end(), static_cast<T>(1), std::multiplies<T>());
};

std::array<long, 6> CalculateCenterFillDimensions(long oldH, long oldW, long h, long w)
{
    long resizedW, resizedH, top, bottom, left, right;
    auto oldHFloat = (float)oldH;
    auto oldWFloat = (float)oldW;
    auto hFloat = (float)h;
    auto wFloat = (float)w;

    auto oldAspectRatio = oldWFloat / oldHFloat;
    auto newAspectRatio = wFloat / hFloat;

    auto scale = (newAspectRatio < oldAspectRatio) ? (hFloat / oldHFloat) : (wFloat / oldWFloat);
    resizedW = (newAspectRatio < oldAspectRatio) ? (long)std::floor(scale * oldWFloat) : w;
    resizedH = (newAspectRatio < oldAspectRatio) ? h : (long)std::floor(scale * oldHFloat);
    long totalPad = (newAspectRatio < oldAspectRatio) ? resizedW - w : resizedH - h;
    long biggerDim = (newAspectRatio < oldAspectRatio) ? w : h;
    long first = (totalPad % 2 == 0) ? totalPad / 2 : (long)std::floor(totalPad / 2.0f);
    long second = first + biggerDim;

    if (newAspectRatio < oldAspectRatio)
    {
        top = 0;
        bottom = h;
        left = first;
        right = second;
    }
    else
    {
        top = first;
        bottom = second;
        left = 0;
        right = w;
    }

    std::array<long, 6> new_dimensions = { resizedW, resizedH, top, bottom, left, right };
    return new_dimensions;
}

Ort::Value CreateTensorValueFromRTVResource(
    OrtDmlApi const& ortDmlApi,
    Ort::MemoryInfo const& memoryInformation,
    ID3D12Resource* d3dResource,
    std::span<const int64_t> tensorDimensions,
    ONNXTensorElementDataType elementDataType,
    /*out*/ void** dmlEpResourceWrapper // Must stay alive with Ort::Value.
)
{
    *dmlEpResourceWrapper = nullptr;

    void* dmlAllocatorResource;
    THROW_IF_NOT_OK(ortDmlApi.CreateGPUAllocationFromD3DResource(d3dResource, &dmlAllocatorResource));
    auto deleter = [&](void*) {ortDmlApi.FreeGPUAllocation(dmlAllocatorResource); };
    deleting_unique_ptr<void> dmlAllocatorResourceCleanup(dmlAllocatorResource, deleter);

    size_t tensorByteSize = static_cast<size_t>(d3dResource->GetDesc().Width * d3dResource->GetDesc().Height
        * 3 * 1 * 4);
    Ort::Value newValue(
        Ort::Value::CreateTensor(
            memoryInformation,
            dmlAllocatorResource,
            tensorByteSize,
            tensorDimensions.data(),
            tensorDimensions.size(),
            elementDataType
        )
    );

    // Return values and the wrapped resource.
    // TODO: Is there some way to get Ort::Value to just own the D3DResource
    // directly so that it gets freed after execution or session destruction?
    *dmlEpResourceWrapper = dmlAllocatorResource;
    dmlAllocatorResourceCleanup.release();

    return newValue;
}

Ort::Value CreateTensorValueFromExistingD3DResource(
    OrtDmlApi const& ortDmlApi,
    Ort::MemoryInfo const& memoryInformation,
    ID3D12Resource* d3dResource,
    std::span<const int64_t> tensorDimensions,
    ONNXTensorElementDataType elementDataType,
    /*out*/ void** dmlEpResourceWrapper // Must stay alive with Ort::Value.
)
{
    *dmlEpResourceWrapper = nullptr;

    void* dmlAllocatorResource;
    THROW_IF_NOT_OK(ortDmlApi.CreateGPUAllocationFromD3DResource(d3dResource, &dmlAllocatorResource));
    auto deleter = [&](void*) {ortDmlApi.FreeGPUAllocation(dmlAllocatorResource); };
    deleting_unique_ptr<void> dmlAllocatorResourceCleanup(dmlAllocatorResource, deleter);

    size_t tensorByteSize = static_cast<size_t>(d3dResource->GetDesc().Width);
    Ort::Value newValue(
        Ort::Value::CreateTensor(
            memoryInformation,
            dmlAllocatorResource,
            tensorByteSize,
            tensorDimensions.data(),
            tensorDimensions.size(),
            elementDataType
        )
    );

    // Return values and the wrapped resource.
    // TODO: Is there some way to get Ort::Value to just own the D3DResource
    // directly so that it gets freed after execution or session destruction?
    *dmlEpResourceWrapper = dmlAllocatorResource;
    dmlAllocatorResourceCleanup.release();

    return newValue;
}

Microsoft::WRL::ComPtr<ID3D12Resource> CreateD3D12ResourceForTensor(
    ID3D12Device* d3dDevice,
    size_t elementByteSize,
    std::span<const int64_t> tensorDimensions
)
{
    // Try to allocate the backing memory for the caller
    auto bufferSize = GetElementCount(tensorDimensions);
    size_t bufferByteSize = static_cast<size_t>(bufferSize * elementByteSize);

    // DML needs the resources' sizes to be a multiple of 4 bytes
    (bufferByteSize += 3) &= ~3;

    D3D12_HEAP_PROPERTIES heapProperties = {
        D3D12_HEAP_TYPE_DEFAULT,
        D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
        D3D12_MEMORY_POOL_UNKNOWN,
        0,
        0
    };
    D3D12_RESOURCE_DESC resourceDesc = {
        D3D12_RESOURCE_DIMENSION_BUFFER,
        0,
        static_cast<uint64_t>(bufferByteSize),
        1,
        1,
        1,
        DXGI_FORMAT_UNKNOWN,
        {1, 0},
        D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
    };

    Microsoft::WRL::ComPtr<ID3D12Resource> gpuResource;
    THROW_IF_FAILED(d3dDevice->CreateCommittedResource(
        &heapProperties,
        D3D12_HEAP_FLAG_NONE,
        &resourceDesc,
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        __uuidof(ID3D12Resource),
        /*out*/ &gpuResource
    ));

    return gpuResource;
}

Ort::Value CreateTensorValueUsingD3DResource(
    ID3D12Device* d3d12Device,
    OrtDmlApi const& ortDmlApi,
    Ort::MemoryInfo const& memoryInformation,
    std::span<const int64_t> tensorDimensions,
    ONNXTensorElementDataType elementDataType,
    size_t elementByteSize,
    /*out*/ void** dmlEpResourceWrapper // Must stay alive with Ort::Value.
)
{
    // Create empty resource (values don't matter because we won't read them back anyway).
    Microsoft::WRL::ComPtr<ID3D12Resource> d3dResource = CreateD3D12ResourceForTensor(
        d3d12Device,
        sizeof(float),
        tensorDimensions
    );

    return CreateTensorValueFromExistingD3DResource(
        ortDmlApi,
        memoryInformation,
        d3dResource.Get(),
        tensorDimensions,
        ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT,
        /*out*/ dmlEpResourceWrapper
    );
}

std::vector<float> Eval(Ort::Session& session, const Ort::Value& prev_input) {
    OutputDebugString(L"In EvalORTInference");
    // Squeezenet opset v7 https://github.com/onnx/models/blob/master/vision/classification/squeezenet/README.md
    const wchar_t* modelFilePath = L"C:/Users/numform/Windows-Machine-Learning/Samples/WinMLSamplesGallery/WinMLSamplesGalleryNative/efficientnet-lite4-11.onnx";
    const char* modelInputTensorName = "images:0";
    const char* modelOutputTensorName = "Softmax:0";
    const std::array<int64_t, 4> inputShape = { 1, 224, 224, 3 };
    const std::array<int64_t, 2> outputShape = { 1, 1000 };

    const bool passTensorsAsD3DResources = true;

    LARGE_INTEGER startTime;
    LARGE_INTEGER d3dDeviceCreationTime;
    LARGE_INTEGER sessionCreationTime;
    LARGE_INTEGER tensorCreationTime;
    LARGE_INTEGER bindingTime;
    LARGE_INTEGER runTime;
    LARGE_INTEGER synchronizeOutputsTime;
    LARGE_INTEGER cpuFrequency;
    QueryPerformanceFrequency(&cpuFrequency);
    QueryPerformanceCounter(&startTime);

    try
    {
        Microsoft::WRL::ComPtr<ID3D12Device> d3d12Device;
        THROW_IF_FAILED(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&d3d12Device)));
        QueryPerformanceCounter(&d3dDeviceCreationTime);

        OrtApi const& ortApi = Ort::GetApi(); // Uses ORT_API_VERSION
        const OrtDmlApi* ortDmlApi;
        THROW_IF_NOT_OK(ortApi.GetExecutionProviderApi("DML", ORT_API_VERSION, reinterpret_cast<const void**>(&ortDmlApi)));

        Ort::IoBinding ioBinding = Ort::IoBinding::IoBinding(session);
        const char* memoryInformationName = passTensorsAsD3DResources ? "DML" : "Cpu";
        Ort::MemoryInfo memoryInformation(memoryInformationName, OrtAllocatorType::OrtDeviceAllocator, 0, OrtMemType::OrtMemTypeDefault);

        // Create output tensor on device memory.
        Ort::Value outputTensor(nullptr);
        std::vector<float> outputTensorValues(static_cast<size_t>(GetElementCount(outputShape)), 0.0f);
        Microsoft::WRL::ComPtr<IUnknown> outputTensorEpWrapper;

        const char* otMemoryInformationName = "Cpu";
        Ort::MemoryInfo otMemoryInformation(otMemoryInformationName, OrtAllocatorType::OrtDeviceAllocator, 0, OrtMemType::OrtMemTypeDefault);
        outputTensor = Ort::Value::CreateTensor<float>(otMemoryInformation, outputTensorValues.data(), outputTensorValues.size(), outputShape.data(), 2);

        QueryPerformanceCounter(&tensorCreationTime);

        ////////////////////////////////////////
        // Bind the tensor inputs to the model, and run it.
        ioBinding.BindInput(modelInputTensorName, prev_input);
        ioBinding.BindOutput(modelOutputTensorName, outputTensor);
        ioBinding.SynchronizeInputs();
        QueryPerformanceCounter(&bindingTime);

        Ort::RunOptions runOptions;

        // TODO: Upload inputTensorValues to GPU inputTensor.

        session.Run(runOptions, ioBinding);
        OutputDebugString(L"Done evaluating inference session");
        QueryPerformanceCounter(&runTime);
        printf("Synchronizing outputs.\n");
        ioBinding.SynchronizeOutputs();
        QueryPerformanceCounter(&synchronizeOutputsTime);
        printf("Finished execution.\n");

        for (int i = 0; i <= std::min(outputTensorValues.size(), size_t(10)); ++i)
        {
            std::wstring value = std::to_wstring(outputTensorValues[i]);
            std::wstring i_w_str = std::to_wstring(i);
            OutputDebugString(L"Output[");
            OutputDebugString(i_w_str.c_str());
            OutputDebugString(L"]: ");
            OutputDebugString(value.c_str());
            OutputDebugString(L"\n");
        }
        std::vector<uint32_t> indices(outputTensorValues.size(), 0);
        std::iota(indices.begin(), indices.end(), 0);
        sort(
            indices.begin(),
            indices.end(),
            [&](uint32_t a, uint32_t b)
            {
                return (outputTensorValues[a] > outputTensorValues[b]);
            }
        );
        OutputDebugString(L"Top 10:");
        OutputDebugString(L"\n");

        std::vector<float> top_10;
        for (int i = 0; i <= std::min(indices.size(), size_t(10)); ++i)
        {
            std::wstring first = std::to_wstring(indices[i]);
            std::wstring second = std::to_wstring(outputTensorValues[indices[i]]);
            top_10.push_back(outputTensorValues[indices[i]]);

            printf("output[%d] = %f\n", indices[i], outputTensorValues[indices[i]]);
            OutputDebugString(L"Output[");
            OutputDebugString(first.c_str());
            OutputDebugString(L"]: ");
            OutputDebugString(second.c_str());
            OutputDebugString(L"\n");
        }
        //return top_10;
        return outputTensorValues;
    }
    catch (Ort::Exception const& exception)
    {
        printf("Error running model inference: %s\n", exception.what());
        //return EXIT_FAILURE;
    }
    catch (std::exception const& exception)
    {
        printf("Error running model inference: %s\n", exception.what());
        //return EXIT_FAILURE;
    }

}

winrt::com_array<float> Preprocess(Ort::Session& session,
    Ort::Session& inferenceSession,
    ID3D12Device*& device,
    bool& Running,
    IDXGISwapChain3*& swapChain,
    int& frameIndex,
    ID3D12CommandAllocator* commandAllocator[],
    ID3D12GraphicsCommandList*& commandList,
    ID3D12CommandQueue*& commandQueue)
{
    OutputDebugString(L"In Preprocess");

    long newH = 224;
    long newW = 224;
    long h = 512;
    long w = 512;
    std::array<long, 6> center_fill_dimensions = CalculateCenterFillDimensions(h, w, newH, newW);
    long resizedW = center_fill_dimensions[0];
    long resizedH = center_fill_dimensions[1];
    long top = center_fill_dimensions[2];
    long bottom = center_fill_dimensions[3];
    long left = center_fill_dimensions[4];
    long right = center_fill_dimensions[5];
    winrt::hstring interpolationMode = L"nearest";
    long c = 3;
    const std::array<int64_t, 4> preprocessInputShape = { 1, 512, 512, 4 };
    const std::array<int64_t, 4> preprocessOutputShape = { 1, 224, 224, 3 };

    auto resize_op = LearningModelOperator(L"Resize")
        .SetInput(L"X", L"Input")
        .SetConstant(L"roi", TensorFloat::CreateFromIterable({ 8 }, { 0, 0, 0, 0, 1, 1, 1, 1 }))
        .SetConstant(L"scales", TensorFloat::CreateFromIterable({ 4 }, { 1, (float)(1 + resizedH) / (float)h, (float)(1 + resizedH) / (float)h, 1 }))
        .SetAttribute(L"mode", TensorString::CreateFromArray({}, { interpolationMode }))
        .SetOutput(L"Y", L"ResizeOutput");

    auto slice_op = LearningModelOperator(L"Slice")
        .SetInput(L"data", L"ResizeOutput")
        .SetConstant(L"starts", TensorInt64Bit::CreateFromIterable({ 4 }, { 0, top, left, 0 }))
        .SetConstant(L"ends", TensorInt64Bit::CreateFromIterable({ 4 }, { LLONG_MAX, bottom, right, 3 }))
        .SetOutput(L"output", L"Output");

    //auto dimension_transpose = LearningModelOperator(L"Transpose")
    //    .SetInput(L"data", L"SliceOutput")
    //    .SetAttribute(L"perm", TensorInt64Bit::CreateFromArray({ 4 }, { INT64(0), INT64(3), INT64(1), INT64(2)}))
    //    .SetOutput(L"transposed", L"Output");

    //auto preprocessingModelBuilder =
    //    LearningModelBuilder::Create(12)
    //    .Inputs().Add(LearningModelBuilder::CreateTensorFeatureDescriptor(L"Input", TensorKind::Float, preprocessInputShape))
    //    .Outputs().Add(LearningModelBuilder::CreateTensorFeatureDescriptor(L"Output", TensorKind::Float, preprocessOutputShape))
    //    .Operators().Add(resize_op)
    //    .Operators().Add(slice_op);
    //    //.Operators().Add(dimension_transpose);
    //auto preprocessingModel = preprocessingModelBuilder.CreateModel();

    //preprocessingModelBuilder.Save(L"C:/Users/numform/Windows-Machine-Learning/Samples/WinMLSamplesGallery/WinMLSamplesGalleryNative/dx_preprocessor_efficient_net.onnx");

    // Squeezenet opset v7 https://github.com/onnx/models/blob/master/vision/classification/squeezenet/README.md
    //const wchar_t* modelFilePath = L"./squeezenet1.1-7.onnx";
    const wchar_t* modelFilePath = L"C:/Users/numform/Windows-Machine-Learning/Samples/WinMLSamplesGallery/WinMLSamplesGalleryNative/squeezenet1.1-7.onnx";
    const char* modelInputTensorName = "data";
    const char* modelOutputTensorName = "squeezenet0_flatten0_reshape0";
    const char* preprocessModelInputTensorName = "Input";
    const char* preprocessModelOutputTensorName = "Output";
    // Might have to change the 3's below to 4 for rgba
    //const std::array<int64_t, 4> preprocessInputShape = { 1, 512, 512, 4 };
    //const std::array<int64_t, 4> preprocessOutputShape = { 1, 3, 224, 224 };

    HRESULT hr;
    ID3D12Resource* new_buffer;
    ID3D12Resource* current_buffer;

    D3D12_RESOURCE_DESC resourceDesc = {
        D3D12_RESOURCE_DIMENSION_BUFFER,
        0,
        static_cast<uint64_t>(800 * 600 * 3 * 4),
        1,
        1,
        1,
        DXGI_FORMAT_UNKNOWN,
        {1, 0},
        D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
    };

    const CD3DX12_HEAP_PROPERTIES default_heap(D3D12_HEAP_TYPE_DEFAULT);
    hr = device->CreateCommittedResource(
        &default_heap, // a default heap
        D3D12_HEAP_FLAG_NONE, // no flags
        &resourceDesc, // resource description for a buffer
        D3D12_RESOURCE_STATE_COPY_DEST, // we will start this heap in the copy destination state since we will copy data
                                        // from the upload heap to this heap
        nullptr, // optimized clear value must be null for this type of resource. used for render targets and depth/stencil buffers
        IID_PPV_ARGS(&new_buffer));
    if (FAILED(hr))
    {
        Running = false;
        //return false;
    }

    hr = swapChain->GetBuffer(frameIndex, IID_PPV_ARGS(&current_buffer));
    auto buffer_desc = current_buffer->GetDesc();

    if (FAILED(hr))
    {
        OutputDebugString(L"Failed to get buffer");
        //return false;
    }

    SaveWICTextureToFile(commandQueue, current_buffer,
        GUID_ContainerFormatJpeg, L"C:/Users/numform/Windows-Machine-Learning/Samples/WinMLSamplesGallery/WinMLSamplesGalleryNative/current_cube_image.jpg",
        D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_PRESENT);

    const auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(current_buffer, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_COPY_SOURCE);

    commandAllocator[frameIndex]->Reset();
    commandList->CopyResource(new_buffer, current_buffer);

    auto new_buffer_desc = new_buffer->GetDesc();

    //long newH = 224;
    //long newW = 224;
    //long h = 512;
    //long w = 512;
    //std::array<long, 6> center_fill_dimensions = CalculateCenterFillDimensions(h, w, newH, newW);
    //long resizedW = center_fill_dimensions[0];
    //long resizedH = center_fill_dimensions[1];
    //long top = center_fill_dimensions[2];
    //long bottom = center_fill_dimensions[3];
    //long left = center_fill_dimensions[4];
    //long right = center_fill_dimensions[5];
    //winrt::hstring interpolationMode = L"nearest";
    //long c = 3;

    const wchar_t* preprocessingModelFilePath = L"C:/Users/numform/Windows-Machine-Learning/Samples/WinMLSamplesGallery/WinMLSamplesGalleryNative/dx_preprocessor.onnx";

    const bool passTensorsAsD3DResources = true;

    LARGE_INTEGER startTime;
    LARGE_INTEGER d3dDeviceCreationTime;
    LARGE_INTEGER sessionCreationTime;
    LARGE_INTEGER tensorCreationTime;
    LARGE_INTEGER bindingTime;
    LARGE_INTEGER runTime;
    LARGE_INTEGER synchronizeOutputsTime;
    LARGE_INTEGER cpuFrequency;
    QueryPerformanceFrequency(&cpuFrequency);
    QueryPerformanceCounter(&startTime);

    try
    {
        ComPtr<ID3D12Device> d3d12Device;
        THROW_IF_FAILED(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&d3d12Device)));
        QueryPerformanceCounter(&d3dDeviceCreationTime);

        OrtApi const& ortApi = Ort::GetApi(); // Uses ORT_API_VERSION
        const OrtDmlApi* ortDmlApi;
        THROW_IF_NOT_OK(ortApi.GetExecutionProviderApi("DML", ORT_API_VERSION, reinterpret_cast<const void**>(&ortDmlApi)));

        QueryPerformanceCounter(&sessionCreationTime);

        Ort::IoBinding ioBinding = Ort::IoBinding::IoBinding(session);
        const char* memoryInformationName = passTensorsAsD3DResources ? "DML" : "Cpu";
        Ort::MemoryInfo memoryInformation(memoryInformationName, OrtAllocatorType::OrtDeviceAllocator, 0, OrtMemType::OrtMemTypeDefault);

        ComPtr<IUnknown> inputTensorEpWrapper;

        Ort::Value inputTensor = CreateTensorValueFromRTVResource(
            *ortDmlApi,
            memoryInformation,
            new_buffer,
            preprocessInputShape,
            ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT,
            /*out*/ IID_PPV_ARGS_Helper(inputTensorEpWrapper.GetAddressOf())
        );

        Ort::Value outputTensor(nullptr);
        std::vector<float> outputTensorValues(static_cast<size_t>(GetElementCount(preprocessOutputShape)), 0.0f);
        ComPtr<IUnknown> outputTensorEpWrapper;

        outputTensor = CreateTensorValueUsingD3DResource(
            d3d12Device.Get(),
            *ortDmlApi,
            memoryInformation,
            preprocessOutputShape,
            ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT,
            sizeof(float),
            /*out*/ IID_PPV_ARGS_Helper(outputTensorEpWrapper.GetAddressOf())
        );

        QueryPerformanceCounter(&tensorCreationTime);

        ////////////////////////////////////////
        // Bind the tensor inputs to the model, and run it.
        ioBinding.BindInput(preprocessModelInputTensorName, inputTensor);
        ioBinding.BindOutput(preprocessModelOutputTensorName, outputTensor);
        ioBinding.SynchronizeInputs();
        QueryPerformanceCounter(&bindingTime);

        Ort::RunOptions runOptions;

        // TODO: Upload inputTensorValues to GPU inputTensor.

        printf("Beginning execution.\n");
        printf("Running Session.\n");
        session.Run(runOptions, ioBinding);
        OutputDebugString(L"Done evaluating preprocessing session");
        //ioBinding.SynchronizeOutputs();
        QueryPerformanceCounter(&synchronizeOutputsTime);

        auto eval_results_std = Eval(inferenceSession, outputTensor);
        winrt::com_array<float> eval_results(1000);
        for (int i = 0; i < 1000; i++) {
            eval_results[i] = eval_results_std[i];
        }
        return eval_results;
    }
    catch (Ort::Exception const& exception)
    {
        printf("Error running model inference: %s\n", exception.what());
        //return EXIT_FAILURE;
    }
    catch (std::exception const& exception)
    {
        printf("Error running model inference: %s\n", exception.what());
        //return EXIT_FAILURE;
    }

}
