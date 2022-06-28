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

Ort::Session CreateSession(const wchar_t* model_file_path)
{
    OrtApi const& ortApi = Ort::GetApi();
    const OrtDmlApi* ortDmlApi;
    ortApi.GetExecutionProviderApi("DML", ORT_API_VERSION, reinterpret_cast<const void**>(&ortDmlApi));
    Ort::Env ortEnvironment(ORT_LOGGING_LEVEL_WARNING, "DirectML_Direct3D_TensorAllocation_Test");
    Ort::SessionOptions sessionOptions;
    sessionOptions.SetExecutionMode(ExecutionMode::ORT_SEQUENTIAL);
    sessionOptions.DisableMemPattern();
    sessionOptions.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
    ortApi.AddFreeDimensionOverrideByName(sessionOptions, "batch_size", 1);
    OrtSessionOptionsAppendExecutionProvider_DML(sessionOptions, 0);

    return Ort::Session(ortEnvironment, model_file_path, sessionOptions);
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
        * 3 * 4);
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
    const char* modelInputTensorName = "images:0";
    const char* modelOutputTensorName = "Softmax:0";
    const std::array<int64_t, 4> inputShape = { 1, 224, 224, 3 };
    const std::array<int64_t, 2> outputShape = { 1, 1000 };

    try
    {
        Microsoft::WRL::ComPtr<ID3D12Device> d3d12Device;
        THROW_IF_FAILED(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&d3d12Device)));

        OrtApi const& ortApi = Ort::GetApi(); // Uses ORT_API_VERSION
        const OrtDmlApi* ortDmlApi;
        THROW_IF_NOT_OK(ortApi.GetExecutionProviderApi("DML", ORT_API_VERSION, reinterpret_cast<const void**>(&ortDmlApi)));

        Ort::IoBinding ioBinding = Ort::IoBinding::IoBinding(session);
        const char* memoryInformationName = "DML";
        Ort::MemoryInfo memoryInformation(memoryInformationName, OrtAllocatorType::OrtDeviceAllocator, 0, OrtMemType::OrtMemTypeDefault);

        // Create output tensor on device memory.
        Ort::Value outputTensor(nullptr);
        std::vector<float> outputTensorValues(static_cast<size_t>(GetElementCount(outputShape)), 0.0f);
        Microsoft::WRL::ComPtr<IUnknown> outputTensorEpWrapper;

        const char* otMemoryInformationName = "Cpu";
        Ort::MemoryInfo otMemoryInformation(otMemoryInformationName, OrtAllocatorType::OrtDeviceAllocator, 0, OrtMemType::OrtMemTypeDefault);
        outputTensor = Ort::Value::CreateTensor<float>(otMemoryInformation, outputTensorValues.data(), outputTensorValues.size(), outputShape.data(), 2);


        std::vector<const char*> input_node_names;
        input_node_names.push_back(modelInputTensorName);
        std::vector<const char*> output_node_names;
        output_node_names.push_back(modelOutputTensorName);

        auto output_tensors = session.Run(Ort::RunOptions{ nullptr }, input_node_names.data(), &prev_input, 1, output_node_names.data(), 1);
        // Get pointer to output tensor float values
        float* floatarr = output_tensors.front().GetTensorMutableData<float>();
        std::vector<float> final_results;
        for (int i = 0; i < 1000; i++) {
            final_results.push_back(floatarr[i]);
        }

        return final_results;
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
    ID3D12Device* device,
    bool& Running,
    IDXGISwapChain3* swapChain,
    UINT frameIndex,
    ID3D12CommandAllocator* commandAllocator,
    ID3D12GraphicsCommandList* commandList,
    ID3D12CommandQueue* commandQueue)
{
    ComPtr<ID3D12CommandAllocator> cmdAlloc;
    ComPtr<ID3D12CommandQueue> cmdQueue;
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    ComPtr<ID3D12GraphicsCommandList> cmdList;
    THROW_IF_FAILED(device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&cmdQueue)));
    THROW_IF_FAILED(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&cmdAlloc)));
    THROW_IF_FAILED(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, cmdAlloc.Get(), nullptr, IID_PPV_ARGS(&cmdList)));

    const char* preprocessModelInputTensorName = "Input";
    const char* preprocessModelOutputTensorName = "Output";
    const std::array<int64_t, 4> preprocessInputShape = { 1, 512, 512, 4 };
    const std::array<int64_t, 4> preprocessOutputShape = { 1, 224, 224, 3 };

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

    auto float_size = sizeof(float);

    if (FAILED(hr))
    {
        OutputDebugString(L"Failed to get buffer");
        //return false;
    }

    const auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(current_buffer, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_COPY_SOURCE);
    cmdList->ResourceBarrier(1, &barrier);

    cmdList->CopyResource(new_buffer, current_buffer);
    //cmdList->CopyBufferRegion(new_buffer, 0, current_buffer, 0, resourceDesc.Width);

    auto new_hr = cmdList->Close();
    ID3D12CommandList* ppCommandLists[] = { cmdList.Get() };
    cmdQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    try
    {
        OrtApi const& ortApi = Ort::GetApi(); // Uses ORT_API_VERSION
        const OrtDmlApi* ortDmlApi;
        THROW_IF_NOT_OK(ortApi.GetExecutionProviderApi("DML", ORT_API_VERSION, reinterpret_cast<const void**>(&ortDmlApi)));

        const char* memoryInformationName = "DML";
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

        std::vector<const char*> input_node_names;
        input_node_names.push_back(preprocessModelInputTensorName);
        std::vector<const char*> output_node_names;
        output_node_names.push_back(preprocessModelOutputTensorName);
        
        Ort::Value outputTensor(nullptr);
        session.Run(Ort::RunOptions{ nullptr }, input_node_names.data(),
            &inputTensor, 1, output_node_names.data(), &outputTensor, 1);

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