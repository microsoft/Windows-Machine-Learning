#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#include "pch.h"
#include "DXResourceBinding.h"
#include "DXResourceBinding.g.cpp"
#include <DirectXMath.h>
#include "stdafx.h"
#include <commctrl.h>
#include <mfapi.h>
#include <iostream>

#include <cstdio>
#include <algorithm>
#include <numeric>
#include <functional>
#include <utility>
#include <string_view>
#include <span>
#include <optional>
#include <memory>

#include <windows.h>
#include <d3d12.h>
#include <wrl/client.h>
#include "dml_provider_factory.h"
#include "onnxruntime_cxx_api.h"

#include "winrt/Microsoft.AI.MachineLearning.h"
#include "winrt/Microsoft.AI.MachineLearning.Experimental.h"
using TensorInt64Bit = winrt::Microsoft::AI::MachineLearning::TensorInt64Bit;
using TensorKind = winrt::Microsoft::AI::MachineLearning::TensorKind;
using LearningModelBuilder = winrt::Microsoft::AI::MachineLearning::Experimental::LearningModelBuilder;
using LearningModelOperator = winrt::Microsoft::AI::MachineLearning::Experimental::LearningModelOperator;
using namespace winrt::Microsoft::AI::MachineLearning;

#define THROW_IF_FAILED(hr) {HRESULT localHr = (hr); if (FAILED(hr)) throw hr;}
#define RETURN_IF_FAILED(hr) {HRESULT localHr = (hr); if (FAILED(hr)) return hr;}
#define THROW_IF_NOT_OK(status) {auto localStatus = (status); if (localStatus) throw E_FAIL;}
#define RETURN_HR_IF_NOT_OK(status) {auto localStatus = (status); if (localStatus) return E_FAIL;}

#undef min

Microsoft::WRL::ComPtr<ID3D12Resource> d3dResource;

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

struct Vertex {
    Vertex(float x, float y, float z, float u, float v) : pos(x, y, z), texCoord(u, v) {}
    DirectX::XMFLOAT3 pos;
    DirectX::XMFLOAT2 texCoord;
};

static HMODULE GetCurrentModule()
{ // NB: XP+ solution!
    HMODULE hModule = NULL;
    GetModuleHandleEx(
        GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
        (LPCTSTR)GetCurrentModule,
        &hModule);

    return hModule;
}

namespace winrt::WinMLSamplesGalleryNative::implementation
{
	int DXResourceBinding::BindDXResourcesUsingORT() {
        OutputDebugString(L"Will this output work?");
        //LaunchNewWindow();
        HINSTANCE hInstance = GetCurrentModule();
        StartHWind(hInstance, 10);
		return 0;
	}
}

int WINAPI StartHWind(HINSTANCE hInstance,    //Main windows function
    int nShowCmd)
{
    // create the window
    if (!InitializeWindow(hInstance, nShowCmd, FullScreen))
    {
        MessageBox(0, L"Window Initialization - Failed",
            L"Error", MB_OK);
        return 1;
    }

    // initialize direct3d
    if (!InitD3D())
    {
        MessageBox(0, L"Failed to initialize direct3d 12",
            L"Error", MB_OK);
        Cleanup();
        return 1;
    }

    // start the main loop
    mainloop();

    // we want to wait for the gpu to finish executing the command list before we start releasing everything
    WaitForPreviousFrame();

    // close the fence event
    CloseHandle(fenceEvent);

    // clean up everything
    Cleanup();

    return 0;
}

// create and show the window
bool InitializeWindow(HINSTANCE hInstance,
    int ShowWnd,
    bool fullscreen)

{
    if (fullscreen)
    {
        HMONITOR hmon = MonitorFromWindow(hwnd,
            MONITOR_DEFAULTTONEAREST);
        MONITORINFO mi = { sizeof(mi) };
        GetMonitorInfo(hmon, &mi);

        Width = mi.rcMonitor.right - mi.rcMonitor.left;
        Height = mi.rcMonitor.bottom - mi.rcMonitor.top;
    }

    WNDCLASSEX wc;

    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.cbClsExtra = NULL;
    wc.cbWndExtra = NULL;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 2);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = WindowName;
    wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

    if (!RegisterClassEx(&wc))
    {
        MessageBox(NULL, L"Error registering class",
            L"Error", MB_OK | MB_ICONERROR);
        return false;
    }

    hwnd = CreateWindowEx(NULL,
        WindowName,
        WindowTitle,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        Width, Height,
        NULL,
        NULL,
        hInstance,
        NULL);

    if (!hwnd)
    {
        MessageBox(NULL, L"Error creating window",
            L"Error", MB_OK | MB_ICONERROR);
        return false;
    }

    if (fullscreen)
    {
        SetWindowLong(hwnd, GWL_STYLE, 0);
    }

    ShowWindow(hwnd, ShowWnd);
    UpdateWindow(hwnd);

    return true;
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

int EvalORTInference(const Ort::Value& prev_input) {
    OutputDebugString(L"In EvalORTInferencec");
    // Squeezenet opset v7 https://github.com/onnx/models/blob/master/vision/classification/squeezenet/README.md
    const wchar_t* modelFilePath = L"C:/Users/numform/Windows-Machine-Learning/Samples/WinMLSamplesGallery/WinMLSamplesGalleryNative/squeezenet1.1-7.onnx";
    const char* modelInputTensorName = "data";
    const char* modelOutputTensorName = "squeezenet0_flatten0_reshape0";
    const std::array<int64_t, 4> inputShape = { 1, 3, 224, 224 };
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

        // ONNX Runtime setup
        Ort::Env ortEnvironment(ORT_LOGGING_LEVEL_WARNING, "DirectML_Direct3D_TensorAllocation_Test");
        Ort::SessionOptions sessionOptions;
        sessionOptions.SetExecutionMode(ExecutionMode::ORT_SEQUENTIAL);
        sessionOptions.DisableMemPattern();
        sessionOptions.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
        ortApi.AddFreeDimensionOverrideByName(sessionOptions, "batch_size", 1);
        OrtSessionOptionsAppendExecutionProvider_DML(sessionOptions, 0);
        Ort::Session session = Ort::Session(ortEnvironment, modelFilePath, sessionOptions);
        QueryPerformanceCounter(&sessionCreationTime);

        Ort::IoBinding ioBinding = Ort::IoBinding::IoBinding(session);
        const char* memoryInformationName = passTensorsAsD3DResources ? "DML" : "Cpu";
        Ort::MemoryInfo memoryInformation(memoryInformationName, OrtAllocatorType::OrtDeviceAllocator, 0, OrtMemType::OrtMemTypeDefault);
        // Not needed: Ort::Allocator allocator(session, memoryInformation);

        // Create input tensor.
        //Ort::Value inputTensor(nullptr);
        //std::vector<float> inputTensorValues(static_cast<size_t>(GetElementCount(inputShape)), 0.0f);
        //std::iota(inputTensorValues.begin(), inputTensorValues.end(), 0.0f);
        //Microsoft::WRL::ComPtr<IUnknown> inputTensorEpWrapper;

        //// Create empty D3D resource for input.
        //inputTensor = CreateTensorValueUsingD3DResource(
        //    d3d12Device.Get(),
        //    *ortDmlApi,
        //    memoryInformation,
        //    inputShape,
        //    ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT,
        //    sizeof(float),
        //    /*out*/ IID_PPV_ARGS_Helper(inputTensorEpWrapper.GetAddressOf())
        //);

        // Create output tensor on device memory.
        Ort::Value outputTensor(nullptr);
        std::vector<float> outputTensorValues(static_cast<size_t>(GetElementCount(outputShape)), 0.0f);
        Microsoft::WRL::ComPtr<IUnknown> outputTensorEpWrapper;

        //outputTensor = CreateTensorValueUsingD3DResource(
        //    d3d12Device.Get(),
        //    *ortDmlApi,
        //    memoryInformation,
        //    outputShape,
        //    ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT,
        //    sizeof(float),
        //    /*out*/ IID_PPV_ARGS_Helper(outputTensorEpWrapper.GetAddressOf())
        //);

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


        for (int i = 0; i <= std::min(indices.size(), size_t(10)); ++i)
        {
            std::wstring first = std::to_wstring(indices[i]);
            std::wstring second = std::to_wstring(outputTensorValues[indices[i]]);

            printf("output[%d] = %f\n", indices[i], outputTensorValues[indices[i]]);
            OutputDebugString(L"Output[");
            OutputDebugString(first.c_str());
            OutputDebugString(L"]: ");
            OutputDebugString(second.c_str());
            OutputDebugString(L"\n");
        }

    }
    catch (Ort::Exception const& exception)
    {
        printf("Error running model inference: %s\n", exception.what());
        return EXIT_FAILURE;
    }
    catch (std::exception const& exception)
    {
        printf("Error running model inference: %s\n", exception.what());
        return EXIT_FAILURE;
    }

    return 0;
}

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

    std::array<long, 6> new_dimensions = { resizedW, resizedH, top, bottom, left, right};
    return new_dimensions;
}

int EvalORT()
{
    OutputDebugString(L"In EvalORT");
    // Squeezenet opset v7 https://github.com/onnx/models/blob/master/vision/classification/squeezenet/README.md
    //const wchar_t* modelFilePath = L"./squeezenet1.1-7.onnx";
    const wchar_t* modelFilePath = L"C:/Users/numform/Windows-Machine-Learning/Samples/WinMLSamplesGallery/WinMLSamplesGalleryNative/squeezenet1.1-7.onnx";
    const char* modelInputTensorName = "data";
    const char* modelOutputTensorName = "squeezenet0_flatten0_reshape0";
    const char* preprocessModelInputTensorName = "Input";
    const char* preprocessModelOutputTensorName = "Output";
    // Might have to change the 3's below to 4 for rgba
    const std::array<int64_t, 4> preprocessInputShape = { 1, 512, 512, 4};
    const std::array<int64_t, 4> preprocessOutputShape = { 1, 3, 224, 224 };

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
        return false;
    }

    hr = swapChain->GetBuffer(frameIndex, IID_PPV_ARGS(&current_buffer));
    auto buffer_desc = current_buffer->GetDesc();

    if (FAILED(hr))
    {
        OutputDebugString(L"Failed to get buffer");
        return false;
    }

    const auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(current_buffer, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_COPY_SOURCE);

    commandAllocator[frameIndex]->Reset();
    commandList->CopyResource(new_buffer, current_buffer);

    auto new_buffer_desc = new_buffer->GetDesc();


    //commandList->CopyTextureRegion(new_buffer, 10, 20, 0, pSourceTexture);

    //Try this if it doesn't workwidth = width * height and height = 1

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


    //auto resize_op = LearningModelOperator(L"Resize")
    //    .SetInput(L"X", L"Input")
    //    .SetConstant(L"roi", TensorFloat::CreateFromIterable({ 8 }, { 0, 0, 0, 0, 1, 1, 1, 1 }))
    //    .SetConstant(L"scales", TensorFloat::CreateFromIterable({ 4 }, { 1, (float)(1 + resizedH) / (float)h, (float)(1 + resizedH) / (float)h, 1 }))
    //    .SetAttribute(L"mode", TensorString::CreateFromArray({}, { interpolationMode }))
    //    .SetOutput(L"Y", L"ResizeOutput");

    //auto slice_op = LearningModelOperator(L"Slice")
    //    .SetInput(L"data", L"ResizeOutput")
    //    .SetConstant(L"starts", TensorInt64Bit::CreateFromIterable({ 4 }, { 0, top, left, 0 }))
    //    .SetConstant(L"ends", TensorInt64Bit::CreateFromIterable({ 4 }, { LLONG_MAX, bottom, right, 3 }))
    //    .SetOutput(L"output", L"SliceOutput");

    //auto dimension_transpose = LearningModelOperator(L"Transpose")
    //    .SetInput(L"data", L"SliceOutput")
    //    .SetAttribute(L"perm", TensorInt64Bit::CreateFromArray({ 4 }, { INT64(0), INT64(3), INT64(1), INT64(2)}))
    //    .SetOutput(L"transposed", L"Output");

    //auto preprocessingModelBuilder =
    //    LearningModelBuilder::Create(12)
    //    .Inputs().Add(LearningModelBuilder::CreateTensorFeatureDescriptor(L"Input", TensorKind::Float, preprocessInputShape))
    //    .Outputs().Add(LearningModelBuilder::CreateTensorFeatureDescriptor(L"Output", TensorKind::Float, preprocessOutputShape))
    //    .Operators().Add(resize_op)
    //    .Operators().Add(slice_op)
    //    .Operators().Add(dimension_transpose);
    //auto preprocessingModel = preprocessingModelBuilder.CreateModel();

    //preprocessingModelBuilder.Save(L"C:/Users/numform/Windows-Machine-Learning/Samples/WinMLSamplesGallery/WinMLSamplesGalleryNative/dx_preprocessor.onnx");
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
        Microsoft::WRL::ComPtr<ID3D12Device> d3d12Device;
        THROW_IF_FAILED(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&d3d12Device)));
        QueryPerformanceCounter(&d3dDeviceCreationTime);

        OrtApi const& ortApi = Ort::GetApi(); // Uses ORT_API_VERSION
        const OrtDmlApi* ortDmlApi;
        THROW_IF_NOT_OK(ortApi.GetExecutionProviderApi("DML", ORT_API_VERSION, reinterpret_cast<const void**>(&ortDmlApi)));

        // ONNX Runtime setup
        Ort::Env ortEnvironment(ORT_LOGGING_LEVEL_WARNING, "DirectML_Direct3D_TensorAllocation_Test");
        Ort::SessionOptions sessionOptions;
        sessionOptions.SetExecutionMode(ExecutionMode::ORT_SEQUENTIAL);
        sessionOptions.DisableMemPattern();
        sessionOptions.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
        ortApi.AddFreeDimensionOverrideByName(sessionOptions, "batch_size", 1);
        OrtSessionOptionsAppendExecutionProvider_DML(sessionOptions, 0);
        //Ort::Session session = Ort::Session(ortEnvironment, modelFilePath, sessionOptions);
        Ort::Session session = Ort::Session(ortEnvironment, preprocessingModelFilePath, sessionOptions);

        QueryPerformanceCounter(&sessionCreationTime);

        Ort::IoBinding ioBinding = Ort::IoBinding::IoBinding(session);
        const char* memoryInformationName = passTensorsAsD3DResources ? "DML" : "Cpu";
        Ort::MemoryInfo memoryInformation(memoryInformationName, OrtAllocatorType::OrtDeviceAllocator, 0, OrtMemType::OrtMemTypeDefault);
        // Not needed: Ort::Allocator allocator(session, memoryInformation);

        // Create input tensor.
        //Ort::Value inputTensor(nullptr);
        //std::vector<float> inputTensorValues(static_cast<size_t>(GetElementCount(inferenceInputShape)), 0.0f);
        //std::iota(inputTensorValues.begin(), inputTensorValues.end(), 0.0f);
        Microsoft::WRL::ComPtr<IUnknown> inputTensorEpWrapper;

        //Ort::Value inputTensor(nullptr);
        //std::vector<float> inputTensorValues(static_cast<size_t>(GetElementCount(preprocessInputShape)), 0.0f);
        //std::iota(inputTensorValues.begin(), inputTensorValues.end(), 0.0f);
        //Microsoft::WRL::ComPtr<IUnknown> inputTensorEpWrapper;

        //// Create empty D3D resource for input.
        //inputTensor = CreateTensorValueUsingD3DResource(
        //    d3d12Device.Get(),
        //    *ortDmlApi,
        //    memoryInformation,
        //    preprocessInputShape,
        //    ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT,
        //    sizeof(float),
        //    /*out*/ IID_PPV_ARGS_Helper(inputTensorEpWrapper.GetAddressOf())
        //);

 /*       Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> to_cpy;
        RETURN_IF_FAILED((
            create_resource_barrier_command_list<D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE>(
                d3d12Device.Get(),
                commandQueue,
                commandAllocator,
                position_buffer_.Get(),
                &to_cpy)));

        ID3D12CommandList* const to_cpy_list[] = {
            to_cpy.Get()
        };*/

        //commandQueue->ExecuteCommandLists(_countof(to_cpy_list), to_cpy_list);

        Ort::Value inputTensor = CreateTensorValueFromRTVResource(
            *ortDmlApi,
            memoryInformation,
            new_buffer,
            preprocessInputShape,
            ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT,
            /*out*/ IID_PPV_ARGS_Helper(inputTensorEpWrapper.GetAddressOf())
        );

        // Create output tensor on device memory.
        //Ort::Value outputTensor(nullptr);
        //std::vector<float> outputTensorValues(static_cast<size_t>(GetElementCount(inferenceOutputShape)), 0.0f);
        //Microsoft::WRL::ComPtr<IUnknown> outputTensorEpWrapper;

        Ort::Value outputTensor(nullptr);
        std::vector<float> outputTensorValues(static_cast<size_t>(GetElementCount(preprocessOutputShape)), 0.0f);
        Microsoft::WRL::ComPtr<IUnknown> outputTensorEpWrapper;

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
        EvalORTInference(outputTensor);
    }
    catch (Ort::Exception const& exception)
    {
        printf("Error running model inference: %s\n", exception.what());
        return EXIT_FAILURE;
    }
    catch (std::exception const& exception)
    {
        printf("Error running model inference: %s\n", exception.what());
        return EXIT_FAILURE;
    }

    return 0;
}

void mainloop() {
    MSG msg;
    ZeroMemory(&msg, sizeof(MSG));

    //initORT();

    while (Running)
    {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
                break;

            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else {
            // run game code
            Update(); // update the game logic
            CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle = Render(); // execute the command queue (rendering the scene is the result of the gpu executing the command lists)
            d3dResource = textureBuffer;
            //EvalORT(rtvHandle);
            EvalORT();
        }

    }
}



LRESULT CALLBACK WndProc(HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam)

{
    switch (msg)
    {
    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE) {
            if (MessageBox(0, L"Are you sure you want to exit?",
                L"Really?", MB_YESNO | MB_ICONQUESTION) == IDYES)
            {
                Running = false;
                DestroyWindow(hwnd);
            }
        }
        return 0;

    case WM_DESTROY: // x button on top right corner of window was pressed
        Running = false;
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd,
        msg,
        wParam,
        lParam);
}

bool InitD3D()
{
    HRESULT hr;

    // -- Create the Device -- //

    IDXGIFactory4* dxgiFactory;
    hr = CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory));
    if (FAILED(hr))
    {
        return false;
    }

    IDXGIAdapter1* adapter; // adapters are the graphics card (this includes the embedded graphics on the motherboard)

    int adapterIndex = 0; // we'll start looking for directx 12  compatible graphics devices starting at index 0

    bool adapterFound = false; // set this to true when a good one was found

                               // find first hardware gpu that supports d3d 12
    while (dxgiFactory->EnumAdapters1(adapterIndex, &adapter) != DXGI_ERROR_NOT_FOUND)
    {
        DXGI_ADAPTER_DESC1 desc;
        adapter->GetDesc1(&desc);

        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
        {
            // we dont want a software device
            continue;
        }

        // we want a device that is compatible with direct3d 12 (feature level 11 or higher)
        hr = D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr);
        if (SUCCEEDED(hr))
        {
            adapterFound = true;
            break;
        }

        adapterIndex++;
    }

    if (!adapterFound)
    {
        Running = false;
        return false;
    }

    // Create the device
    hr = D3D12CreateDevice(
        adapter,
        D3D_FEATURE_LEVEL_11_0,
        IID_PPV_ARGS(&device)
    );

    if (FAILED(hr))
    {
        Running = false;
        return false;
    }

    // -- Create a direct command queue -- //

    D3D12_COMMAND_QUEUE_DESC cqDesc = {};
    cqDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    cqDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT; // direct means the gpu can directly execute this command queue

    hr = device->CreateCommandQueue(&cqDesc, IID_PPV_ARGS(&commandQueue)); // create the command queue
    if (FAILED(hr))
    {
        Running = false;
        return false;
    }

    // -- Create the Swap Chain (double/tripple buffering) -- //

    DXGI_MODE_DESC backBufferDesc = {}; // this is to describe our display mode
    backBufferDesc.Width = Width; // buffer width
    backBufferDesc.Height = Height; // buffer height
    backBufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // format of the buffer (rgba 32 bits, 8 bits for each chanel)

                                                        // describe our multi-sampling. We are not multi-sampling, so we set the count to 1 (we need at least one sample of course)
    DXGI_SAMPLE_DESC sampleDesc = {};
    sampleDesc.Count = 1; // multisample count (no multisampling, so we just put 1, since we still need 1 sample)

                          // Describe and create the swap chain.
    DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
    swapChainDesc.BufferCount = frameBufferCount; // number of buffers we have
    swapChainDesc.BufferDesc = backBufferDesc; // our back buffer description
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; // this says the pipeline will render to this swap chain
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // dxgi will discard the buffer (data) after we call present
    swapChainDesc.OutputWindow = hwnd; // handle to our window
    swapChainDesc.SampleDesc = sampleDesc; // our multi-sampling description
    swapChainDesc.Windowed = !FullScreen; // set to true, then if in fullscreen must call SetFullScreenState with true for full screen to get uncapped fps

    IDXGISwapChain* tempSwapChain;

    dxgiFactory->CreateSwapChain(
        commandQueue, // the queue will be flushed once the swap chain is created
        &swapChainDesc, // give it the swap chain description we created above
        &tempSwapChain // store the created swap chain in a temp IDXGISwapChain interface
    );

    swapChain = static_cast<IDXGISwapChain3*>(tempSwapChain);

    frameIndex = swapChain->GetCurrentBackBufferIndex();

    // -- Create the Back Buffers (render target views) Descriptor Heap -- //

    // describe an rtv descriptor heap and create
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
    rtvHeapDesc.NumDescriptors = frameBufferCount; // number of descriptors for this heap.
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV; // this heap is a render target view heap

                                                       // This heap will not be directly referenced by the shaders (not shader visible), as this will store the output from the pipeline
                                                       // otherwise we would set the heap's flag to D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    hr = device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvDescriptorHeap));
    if (FAILED(hr))
    {
        Running = false;
        return false;
    }

    // get the size of a descriptor in this heap (this is a rtv heap, so only rtv descriptors should be stored in it.
    // descriptor sizes may vary from device to device, which is why there is no set size and we must ask the 
    // device to give us the size. we will use this size to increment a descriptor handle offset
    rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    // get a handle to the first descriptor in the descriptor heap. a handle is basically a pointer,
    // but we cannot literally use it like a c++ pointer.
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

    // Create a RTV for each buffer (double buffering is two buffers, tripple buffering is 3).
    for (int i = 0; i < frameBufferCount; i++)
    {
        // first we get the n'th buffer in the swap chain and store it in the n'th
        // position of our ID3D12Resource array
        hr = swapChain->GetBuffer(i, IID_PPV_ARGS(&renderTargets[i]));
        if (FAILED(hr))
        {
            Running = false;
            return false;
        }

        // the we "create" a render target view which binds the swap chain buffer (ID3D12Resource[n]) to the rtv handle
        device->CreateRenderTargetView(renderTargets[i], nullptr, rtvHandle);

        // we increment the rtv handle by the rtv descriptor size we got above
        rtvHandle.Offset(1, rtvDescriptorSize);
    }

    // transition swapchain buffer to a UAV state
    // create another descriptor heap that has UAV descriptors and have that point to swapchain buffer
    // just need to pass in the buffer (swapchain->GetBuffer) as long as it's in UAV state I should be 
    // able to pass in the buffer
    // to transition the resource create command list pass in number of transitions (1) to resource barrier
    // that takes in d3d12 resource to state and from state (to state - UAV or d3dresourcestateunordered access) from render target
    // after eval queue up another command to move it back to render target


    // -- Create the Command Allocators -- //

    for (int i = 0; i < frameBufferCount; i++)
    {
        hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator[i]));
        if (FAILED(hr))
        {
            Running = false;
            return false;
        }
    }

    // -- Create a Command List -- //

    // create the command list with the first allocator
    hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator[frameIndex], NULL, IID_PPV_ARGS(&commandList));
    if (FAILED(hr))
    {
        Running = false;
        return false;
    }

    // -- Create a Fence & Fence Event -- //

    // create the fences
    for (int i = 0; i < frameBufferCount; i++)
    {
        hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence[i]));
        if (FAILED(hr))
        {
            Running = false;
            return false;
        }
        fenceValue[i] = 0; // set the initial fence value to 0
    }

    // create a handle to a fence event
    fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (fenceEvent == nullptr)
    {
        Running = false;
        return false;
    }

    // create root signature

    // create a root descriptor, which explains where to find the data for this root parameter
    D3D12_ROOT_DESCRIPTOR rootCBVDescriptor;
    rootCBVDescriptor.RegisterSpace = 0;
    rootCBVDescriptor.ShaderRegister = 0;

    // create a descriptor range (descriptor table) and fill it out
    // this is a range of descriptors inside a descriptor heap
    D3D12_DESCRIPTOR_RANGE  descriptorTableRanges[1]; // only one range right now
    descriptorTableRanges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV; // this is a range of shader resource views (descriptors)
    descriptorTableRanges[0].NumDescriptors = 1; // we only have one texture right now, so the range is only 1
    descriptorTableRanges[0].BaseShaderRegister = 0; // start index of the shader registers in the range
    descriptorTableRanges[0].RegisterSpace = 0; // space 0. can usually be zero
    descriptorTableRanges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND; // this appends the range to the end of the root signature descriptor tables

    // create a descriptor table
    D3D12_ROOT_DESCRIPTOR_TABLE descriptorTable;
    descriptorTable.NumDescriptorRanges = _countof(descriptorTableRanges); // we only have one range
    descriptorTable.pDescriptorRanges = &descriptorTableRanges[0]; // the pointer to the beginning of our ranges array

    // create a root parameter for the root descriptor and fill it out
    D3D12_ROOT_PARAMETER  rootParameters[2]; // two root parameters
    rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV; // this is a constant buffer view root descriptor
    rootParameters[0].Descriptor = rootCBVDescriptor; // this is the root descriptor for this root parameter
    rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX; // our pixel shader will be the only shader accessing this parameter for now

    // fill out the parameter for our descriptor table. Remember it's a good idea to sort parameters by frequency of change. Our constant
    // buffer will be changed multiple times per frame, while our descriptor table will not be changed at all (in this tutorial)
    rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE; // this is a descriptor table
    rootParameters[1].DescriptorTable = descriptorTable; // this is our descriptor table for this root parameter
    rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // our pixel shader will be the only shader accessing this parameter for now

    // create a static sampler
    D3D12_STATIC_SAMPLER_DESC sampler = {};
    sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
    sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    sampler.MipLODBias = 0;
    sampler.MaxAnisotropy = 0;
    sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
    sampler.MinLOD = 0.0f;
    sampler.MaxLOD = D3D12_FLOAT32_MAX;
    sampler.ShaderRegister = 0;
    sampler.RegisterSpace = 0;
    sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    rootSignatureDesc.Init(_countof(rootParameters), // we have 2 root parameters
        rootParameters, // a pointer to the beginning of our root parameters array
        1, // we have one static sampler
        &sampler, // a pointer to our static sampler (array)
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | // we can deny shader stages here for better performance
        D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS);

    ID3DBlob* errorBuff; // a buffer holding the error data if any
    ID3DBlob* signature;
    hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &errorBuff);
    if (FAILED(hr))
    {
        OutputDebugStringA((char*)errorBuff->GetBufferPointer());
        return false;
    }

    hr = device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
    if (FAILED(hr))
    {
        return false;
    }

    // create vertex and pixel shaders

    // when debugging, we can compile the shader files at runtime.
    // but for release versions, we can compile the hlsl shaders
    // with fxc.exe to create .cso files, which contain the shader
    // bytecode. We can load the .cso files at runtime to get the
    // shader bytecode, which of course is faster than compiling
    // them at runtime

    // compile vertex shader
    ID3DBlob* vertexShader; // d3d blob for holding vertex shader bytecode
    hr = D3DCompileFromFile(L"C:/Users/numform/Windows-Machine-Learning/Samples/WinMLSamplesGallery/WinMLSamplesGalleryNative/VertexShader.hlsl",
    //hr = D3DCompileFromFile(L"E:\\work\\Windows-Machine-Learning\\Samples\\WinMLSamplesGallery\\WinMLSamplesGalleryNative\\VertexShader.hlsl",
        nullptr,
        nullptr,
        "main",
        "vs_5_0",
        D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
        0,
        &vertexShader,
        &errorBuff);
    if (FAILED(hr))
    {
        OutputDebugStringA((char*)errorBuff->GetBufferPointer());
        Running = false;
        return false;
    }

    // fill out a shader bytecode structure, which is basically just a pointer
    // to the shader bytecode and the size of the shader bytecode
    D3D12_SHADER_BYTECODE vertexShaderBytecode = {};
    vertexShaderBytecode.BytecodeLength = vertexShader->GetBufferSize();
    vertexShaderBytecode.pShaderBytecode = vertexShader->GetBufferPointer();

    // compile pixel shader
    ID3DBlob* pixelShader;
    hr = D3DCompileFromFile(L"C:/Users/numform/Windows-Machine-Learning/Samples/WinMLSamplesGallery/WinMLSamplesGalleryNative/PixelShader.hlsl",
    //hr = D3DCompileFromFile(L"E:\\work\\Windows-Machine-Learning\\Samples\\WinMLSamplesGallery\\WinMLSamplesGalleryNative\\PixelShader.hlsl",
        nullptr,
        nullptr,
        "main",
        "ps_5_0",
        D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
        0,
        &pixelShader,
        &errorBuff);
    if (FAILED(hr))
    {
        OutputDebugStringA((char*)errorBuff->GetBufferPointer());
        Running = false;
        return false;
    }

    // fill out shader bytecode structure for pixel shader
    D3D12_SHADER_BYTECODE pixelShaderBytecode = {};
    pixelShaderBytecode.BytecodeLength = pixelShader->GetBufferSize();
    pixelShaderBytecode.pShaderBytecode = pixelShader->GetBufferPointer();

    // create input layout

    // The input layout is used by the Input Assembler so that it knows
    // how to read the vertex data bound to it.

    D3D12_INPUT_ELEMENT_DESC inputLayout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    // fill out an input layout description structure
    D3D12_INPUT_LAYOUT_DESC inputLayoutDesc = {};

    // we can get the number of elements in an array by "sizeof(array) / sizeof(arrayElementType)"
    inputLayoutDesc.NumElements = sizeof(inputLayout) / sizeof(D3D12_INPUT_ELEMENT_DESC);
    inputLayoutDesc.pInputElementDescs = inputLayout;

    // create a pipeline state object (PSO)

    // In a real application, you will have many pso's. for each different shader
    // or different combinations of shaders, different blend states or different rasterizer states,
    // different topology types (point, line, triangle, patch), or a different number
    // of render targets you will need a pso

    // VS is the only required shader for a pso. You might be wondering when a case would be where
    // you only set the VS. It's possible that you have a pso that only outputs data with the stream
    // output, and not on a render target, which means you would not need anything after the stream
    // output.

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {}; // a structure to define a pso
    psoDesc.InputLayout = inputLayoutDesc; // the structure describing our input layout
    psoDesc.pRootSignature = rootSignature; // the root signature that describes the input data this pso needs
    psoDesc.VS = vertexShaderBytecode; // structure describing where to find the vertex shader bytecode and how large it is
    psoDesc.PS = pixelShaderBytecode; // same as VS but for pixel shader
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE; // type of topology we are drawing
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM; // format of the render target
    psoDesc.SampleDesc = sampleDesc; // must be the same sample description as the swapchain and depth/stencil buffer
    psoDesc.SampleMask = 0xffffffff; // sample mask has to do with multi-sampling. 0xffffffff means point sampling is done
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT); // a default rasterizer state.
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT); // a default blent state.
    psoDesc.NumRenderTargets = 1; // we are only binding one render target
    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT); // a default depth stencil state

    // create the pso
    hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineStateObject));
    if (FAILED(hr))
    {
        Running = false;
        return false;
    }

    // Create vertex buffer

    // a cube
    Vertex vList[] = {
        // front face
        { -0.5f,  0.5f, -0.5f, 0.0f, 0.0f },
        {  0.5f, -0.5f, -0.5f, 1.0f, 1.0f },
        { -0.5f, -0.5f, -0.5f, 0.0f, 1.0f },
        {  0.5f,  0.5f, -0.5f, 1.0f, 0.0f },

        // right side face
        {  0.5f, -0.5f, -0.5f, 0.0f, 1.0f },
        {  0.5f,  0.5f,  0.5f, 1.0f, 0.0f },
        {  0.5f, -0.5f,  0.5f, 1.0f, 1.0f },
        {  0.5f,  0.5f, -0.5f, 0.0f, 0.0f },

        // left side face
        { -0.5f,  0.5f,  0.5f, 0.0f, 0.0f },
        { -0.5f, -0.5f, -0.5f, 1.0f, 1.0f },
        { -0.5f, -0.5f,  0.5f, 0.0f, 1.0f },
        { -0.5f,  0.5f, -0.5f, 1.0f, 0.0f },

        // back face
        {  0.5f,  0.5f,  0.5f, 0.0f, 0.0f },
        { -0.5f, -0.5f,  0.5f, 1.0f, 1.0f },
        {  0.5f, -0.5f,  0.5f, 0.0f, 1.0f },
        { -0.5f,  0.5f,  0.5f, 1.0f, 0.0f },

        // top face
        { -0.5f,  0.5f, -0.5f, 0.0f, 1.0f },
        {  0.5f,  0.5f,  0.5f, 1.0f, 0.0f },
        {  0.5f,  0.5f, -0.5f, 1.0f, 1.0f },
        { -0.5f,  0.5f,  0.5f, 0.0f, 0.0f },

        // bottom face
        {  0.5f, -0.5f,  0.5f, 0.0f, 0.0f },
        { -0.5f, -0.5f, -0.5f, 1.0f, 1.0f },
        {  0.5f, -0.5f, -0.5f, 0.0f, 1.0f },
        { -0.5f, -0.5f,  0.5f, 1.0f, 0.0f },
    };

    int vBufferSize = sizeof(vList);

    // create default heap
    // default heap is memory on the GPU. Only the GPU has access to this memory
    // To get data into this heap, we will have to upload the data using
    // an upload heap
    const CD3DX12_HEAP_PROPERTIES default_heap(D3D12_HEAP_TYPE_DEFAULT);
    const CD3DX12_RESOURCE_DESC buffer_resource_desc = CD3DX12_RESOURCE_DESC::Buffer(vBufferSize);
    hr = device->CreateCommittedResource(
        &default_heap, // a default heap
        D3D12_HEAP_FLAG_NONE, // no flags
        &buffer_resource_desc, // resource description for a buffer
        D3D12_RESOURCE_STATE_COPY_DEST, // we will start this heap in the copy destination state since we will copy data
                                        // from the upload heap to this heap
        nullptr, // optimized clear value must be null for this type of resource. used for render targets and depth/stencil buffers
        IID_PPV_ARGS(&vertexBuffer));
    if (FAILED(hr))
    {
        Running = false;
        return false;
    }

    // we can give resource heaps a name so when we debug with the graphics debugger we know what resource we are looking at
    vertexBuffer->SetName(L"Vertex Buffer Resource Heap");

    // create upload heap
    // upload heaps are used to upload data to the GPU. CPU can write to it, GPU can read from it
    // We will upload the vertex buffer using this heap to the default heap
    ID3D12Resource* vBufferUploadHeap;
    const CD3DX12_HEAP_PROPERTIES constantBufferHeapProps(D3D12_HEAP_TYPE_UPLOAD);
    const CD3DX12_RESOURCE_DESC constantBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(vBufferSize);
    hr = device->CreateCommittedResource(
        &constantBufferHeapProps, // upload heap
        D3D12_HEAP_FLAG_NONE, // no flags
        &constantBufferDesc, // resource description for a buffer
        D3D12_RESOURCE_STATE_GENERIC_READ, // GPU will read from this buffer and copy its contents to the default heap
        nullptr,
        IID_PPV_ARGS(&vBufferUploadHeap));
    if (FAILED(hr))
    {
        Running = false;
        return false;
    }
    vBufferUploadHeap->SetName(L"Vertex Buffer Upload Resource Heap");

    // store vertex buffer in upload heap
    D3D12_SUBRESOURCE_DATA vertexData = {};
    vertexData.pData = reinterpret_cast<BYTE*>(vList); // pointer to our vertex array
    vertexData.RowPitch = vBufferSize; // size of all our triangle vertex data
    vertexData.SlicePitch = vBufferSize; // also the size of our triangle vertex data

    // we are now creating a command with the command list to copy the data from
    // the upload heap to the default heap
    UpdateSubresources(commandList, vertexBuffer, vBufferUploadHeap, 0, 0, 1, &vertexData);

    // transition the vertex buffer data from copy destination state to vertex buffer state
    const auto v_buffer_state_barrier = CD3DX12_RESOURCE_BARRIER::Transition(vertexBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
    commandList->ResourceBarrier(1, &v_buffer_state_barrier);

    // if dml breaks create another buffer (createcommitedresource) specifying the width as width * height * depthand copy from the render target to that buffer

    //create a tensor of the size of my window (n, h, w, c) 4 as c and just try to pass the buffer into the moodel
    // then do the model builder work to take in n,h,w,c
    // once I have that figure out how to get the actual render target to be interpreted as n, h, w, c
    // already a model that accepts arbitrary height and width

    // Create index buffer

    // a quad (2 triangles)
    DWORD iList[] = {
        // ffront face
        0, 1, 2, // first triangle
        0, 3, 1, // second triangle

        // left face
        4, 5, 6, // first triangle
        4, 7, 5, // second triangle

        // right face
        8, 9, 10, // first triangle
        8, 11, 9, // second triangle

        // back face
        12, 13, 14, // first triangle
        12, 15, 13, // second triangle

        // top face
        16, 17, 18, // first triangle
        16, 19, 17, // second triangle

        // bottom face
        20, 21, 22, // first triangle
        20, 23, 21, // second triangle
    };

    int iBufferSize = sizeof(iList);

    numCubeIndices = sizeof(iList) / sizeof(DWORD);

    const CD3DX12_HEAP_PROPERTIES index_buffer_default_heap(D3D12_HEAP_TYPE_DEFAULT);
    const CD3DX12_RESOURCE_DESC index_buffer_resource_desc = CD3DX12_RESOURCE_DESC::Buffer(iBufferSize);
    // create default heap to hold index buffer
    hr = device->CreateCommittedResource(
        &index_buffer_default_heap, // a default heap
        D3D12_HEAP_FLAG_NONE, // no flags
        &index_buffer_resource_desc, // resource description for a buffer
        D3D12_RESOURCE_STATE_COPY_DEST, // start in the copy destination state
        nullptr, // optimized clear value must be null for this type of resource
        IID_PPV_ARGS(&indexBuffer));
    if (FAILED(hr))
    {
        Running = false;
        return false;
    }

    // we can give resource heaps a name so when we debug with the graphics debugger we know what resource we are looking at
    vertexBuffer->SetName(L"Index Buffer Resource Heap");

    // create upload heap to upload index buffer
    const CD3DX12_HEAP_PROPERTIES ibuffer_upload_heap_default(D3D12_HEAP_TYPE_UPLOAD);
    const CD3DX12_RESOURCE_DESC ibuffer_upload_heap_resource_descp = CD3DX12_RESOURCE_DESC::Buffer(vBufferSize);
    ID3D12Resource* iBufferUploadHeap;
    hr = device->CreateCommittedResource(
        &ibuffer_upload_heap_default, // upload heap
        D3D12_HEAP_FLAG_NONE, // no flags
        &ibuffer_upload_heap_resource_descp, // resource description for a buffer
        D3D12_RESOURCE_STATE_GENERIC_READ, // GPU will read from this buffer and copy its contents to the default heap
        nullptr,
        IID_PPV_ARGS(&iBufferUploadHeap));
    if (FAILED(hr))
    {
        Running = false;
        return false;
    }
    vBufferUploadHeap->SetName(L"Index Buffer Upload Resource Heap");

    // store vertex buffer in upload heap
    D3D12_SUBRESOURCE_DATA indexData = {};
    indexData.pData = reinterpret_cast<BYTE*>(iList); // pointer to our index array
    indexData.RowPitch = iBufferSize; // size of all our index buffer
    indexData.SlicePitch = iBufferSize; // also the size of our index buffer

    // we are now creating a command with the command list to copy the data from
    // the upload heap to the default heap
    UpdateSubresources(commandList, indexBuffer, iBufferUploadHeap, 0, 0, 1, &indexData);

    // transition the vertex buffer data from copy destination state to vertex buffer state
    const auto index_buffer_state_barrier = CD3DX12_RESOURCE_BARRIER::Transition(indexBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
    commandList->ResourceBarrier(1, &index_buffer_state_barrier);

    // Create the depth/stencil buffer

    // create a depth stencil descriptor heap so we can get a pointer to the depth stencil buffer
    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
    dsvHeapDesc.NumDescriptors = 1;
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    hr = device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&dsDescriptorHeap));
    if (FAILED(hr))
    {
        Running = false;
        return false;
    }

    D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc = {};
    depthStencilDesc.Format = DXGI_FORMAT_D32_FLOAT;
    depthStencilDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    depthStencilDesc.Flags = D3D12_DSV_FLAG_NONE;

    D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
    depthOptimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
    depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
    depthOptimizedClearValue.DepthStencil.Stencil = 0;

    const CD3DX12_HEAP_PROPERTIES heap_prop_default(D3D12_HEAP_TYPE_DEFAULT);
    const CD3DX12_RESOURCE_DESC text_2D_resource_desp = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, Width, Height, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
    hr = device->CreateCommittedResource(
        &heap_prop_default,
        D3D12_HEAP_FLAG_NONE,
        &text_2D_resource_desp,
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &depthOptimizedClearValue,
        IID_PPV_ARGS(&depthStencilBuffer)
    );
    if (FAILED(hr))
    {
        Running = false;
        return false;
    }
    dsDescriptorHeap->SetName(L"Depth/Stencil Resource Heap");

    device->CreateDepthStencilView(depthStencilBuffer, &depthStencilDesc, dsDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

    // create the constant buffer resource heap
    // We will update the constant buffer one or more times per frame, so we will use only an upload heap
    // unlike previously we used an upload heap to upload the vertex and index data, and then copied over
    // to a default heap. If you plan to use a resource for more than a couple frames, it is usually more
    // efficient to copy to a default heap where it stays on the gpu. In this case, our constant buffer
    // will be modified and uploaded at least once per frame, so we only use an upload heap

    // first we will create a resource heap (upload heap) for each frame for the cubes constant buffers
    // As you can see, we are allocating 64KB for each resource we create. Buffer resource heaps must be
    // an alignment of 64KB. We are creating 3 resources, one for each frame. Each constant buffer is 
    // only a 4x4 matrix of floats in this tutorial. So with a float being 4 bytes, we have 
    // 16 floats in one constant buffer, and we will store 2 constant buffers in each
    // heap, one for each cube, thats only 64x2 bits, or 128 bits we are using for each
    // resource, and each resource must be at least 64KB (65536 bits)
    for (int i = 0; i < frameBufferCount; ++i)
    {
        // create resource for cube 1
        const CD3DX12_HEAP_PROPERTIES cube_1_upload_heap(D3D12_HEAP_TYPE_UPLOAD);
        const CD3DX12_RESOURCE_DESC cube_1_resource_buffer = CD3DX12_RESOURCE_DESC::Buffer(D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);
        hr = device->CreateCommittedResource(
            &cube_1_upload_heap, // this heap will be used to upload the constant buffer data
            D3D12_HEAP_FLAG_NONE, // no flags
            &cube_1_resource_buffer, // size of the resource heap. Must be a multiple of 64KB for single-textures and constant buffers
            D3D12_RESOURCE_STATE_GENERIC_READ, // will be data that is read from so we keep it in the generic read state
            nullptr, // we do not have use an optimized clear value for constant buffers
            IID_PPV_ARGS(&constantBufferUploadHeaps[i]));
        if (FAILED(hr))
        {
            Running = false;
            return false;
        }
        constantBufferUploadHeaps[i]->SetName(L"Constant Buffer Upload Resource Heap");

        ZeroMemory(&cbPerObject, sizeof(cbPerObject));

        CD3DX12_RANGE readRange(0, 0);    // We do not intend to read from this resource on the CPU. (so end is less than or equal to begin)

        // map the resource heap to get a gpu virtual address to the beginning of the heap
        hr = constantBufferUploadHeaps[i]->Map(0, &readRange, reinterpret_cast<void**>(&cbvGPUAddress[i]));

        // Because of the constant read alignment requirements, constant buffer views must be 256 bit aligned. Our buffers are smaller than 256 bits,
        // so we need to add spacing between the two buffers, so that the second buffer starts at 256 bits from the beginning of the resource heap.
        memcpy(cbvGPUAddress[i], &cbPerObject, sizeof(cbPerObject)); // cube1's constant buffer data
        memcpy(cbvGPUAddress[i] + ConstantBufferPerObjectAlignedSize, &cbPerObject, sizeof(cbPerObject)); // cube2's constant buffer data
    }

    // load the image, create a texture resource and descriptor heap

    // Load the image from file
    D3D12_RESOURCE_DESC textureDesc;
    int imageBytesPerRow;
    BYTE* imageData;
    int imageSize = LoadImageDataFromFile(&imageData, textureDesc, L"C:/Users/numform/Windows-Machine-Learning/Samples/WinMLSamplesGallery/WinMLSamplesGalleryNative/cat.jpg", imageBytesPerRow);
    //int imageSize = LoadImageDataFromFile(&imageData, textureDesc, L"e:\\cat.jpg", imageBytesPerRow);

    //interpret as byte buffer instead of file
    //copy output buffer to another buffer that I've created


    // make sure we have data
    if (imageSize <= 0)
    {
        Running = false;
        return false;
    }

    // create a default heap where the upload heap will copy its contents into (contents being the texture)
    const CD3DX12_HEAP_PROPERTIES texture_default_heap(D3D12_HEAP_TYPE_DEFAULT);
    hr = device->CreateCommittedResource(
        &texture_default_heap, // a default heap
        D3D12_HEAP_FLAG_NONE, // no flags
        &textureDesc, // the description of our texture
        D3D12_RESOURCE_STATE_COPY_DEST, // We will copy the texture from the upload heap to here, so we start it out in a copy dest state
        nullptr, // used for render targets and depth/stencil buffers
        IID_PPV_ARGS(&textureBuffer));
    if (FAILED(hr))
    {
        Running = false;
        return false;
    }
    textureBuffer->SetName(L"Texture Buffer Resource Heap");

    UINT64 textureUploadBufferSize;
    // this function gets the size an upload buffer needs to be to upload a texture to the gpu.
    // each row must be 256 byte aligned except for the last row, which can just be the size in bytes of the row
    // eg. textureUploadBufferSize = ((((width * numBytesPerPixel) + 255) & ~255) * (height - 1)) + (width * numBytesPerPixel);
    //textureUploadBufferSize = (((imageBytesPerRow + 255) & ~255) * (textureDesc.Height - 1)) + imageBytesPerRow;
    device->GetCopyableFootprints(&textureDesc, 0, 1, 0, nullptr, nullptr, nullptr, &textureUploadBufferSize);

    // now we create an upload heap to upload our texture to the GPU
    const CD3DX12_HEAP_PROPERTIES gpu_texture_upload_heap(D3D12_HEAP_TYPE_UPLOAD);
    const CD3DX12_RESOURCE_DESC gpu_tex_resource_description = CD3DX12_RESOURCE_DESC::Buffer(textureUploadBufferSize);
    hr = device->CreateCommittedResource(
        &gpu_texture_upload_heap, // upload heap
        D3D12_HEAP_FLAG_NONE, // no flags
        &gpu_tex_resource_description, // resource description for a buffer (storing the image data in this heap just to copy to the default heap)
        D3D12_RESOURCE_STATE_GENERIC_READ, // We will copy the contents from this heap to the default heap above
        nullptr,
        IID_PPV_ARGS(&textureBufferUploadHeap));
    if (FAILED(hr))
    {
        Running = false;
        return false;
    }
    textureBufferUploadHeap->SetName(L"Texture Buffer Upload Resource Heap");

    // store vertex buffer in upload heap
    D3D12_SUBRESOURCE_DATA textureData = {};
    textureData.pData = &imageData[0]; // pointer to our image data
    textureData.RowPitch = imageBytesPerRow; // size of all our triangle vertex data
    textureData.SlicePitch = imageBytesPerRow * textureDesc.Height; // also the size of our triangle vertex data

    // Now we copy the upload buffer contents to the default heap
    UpdateSubresources(commandList, textureBuffer, textureBufferUploadHeap, 0, 0, 1, &textureData);

    // transition the texture default heap to a pixel shader resource (we will be sampling from this heap in the pixel shader to get the color of pixels)
    const auto pixel_shader_resource_barrier = CD3DX12_RESOURCE_BARRIER::Transition(textureBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    commandList->ResourceBarrier(1, &pixel_shader_resource_barrier);

    // create the descriptor heap that will store our srv
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.NumDescriptors = 1;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    hr = device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&mainDescriptorHeap));
    if (FAILED(hr))
    {
        Running = false;
    }

    // now we create a shader resource view (descriptor that points to the texture and describes it)
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = textureDesc.Format;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    device->CreateShaderResourceView(textureBuffer, &srvDesc, mainDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

    // Now we execute the command list to upload the initial assets (triangle data)
    commandList->Close();
    ID3D12CommandList* ppCommandLists[] = { commandList };
    commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    // increment the fence value now, otherwise the buffer might not be uploaded by the time we start drawing
    fenceValue[frameIndex]++;
    hr = commandQueue->Signal(fence[frameIndex], fenceValue[frameIndex]);
    if (FAILED(hr))
    {
        Running = false;
        return false;
    }

    // we are done with image data now that we've uploaded it to the gpu, so free it up
    delete imageData;

    // create a vertex buffer view for the triangle. We get the GPU memory address to the vertex pointer using the GetGPUVirtualAddress() method
    vertexBufferView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
    vertexBufferView.StrideInBytes = sizeof(Vertex);
    vertexBufferView.SizeInBytes = vBufferSize;

    // create a vertex buffer view for the triangle. We get the GPU memory address to the vertex pointer using the GetGPUVirtualAddress() method
    indexBufferView.BufferLocation = indexBuffer->GetGPUVirtualAddress();
    indexBufferView.Format = DXGI_FORMAT_R32_UINT; // 32-bit unsigned integer (this is what a dword is, double word, a word is 2 bytes)
    indexBufferView.SizeInBytes = iBufferSize;

    // Fill out the Viewport
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    viewport.Width = Width;
    viewport.Height = Height;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;

    // Fill out a scissor rect
    scissorRect.left = 0;
    scissorRect.top = 0;
    scissorRect.right = Width;
    scissorRect.bottom = Height;

    // build projection and view matrix
    XMMATRIX tmpMat = XMMatrixPerspectiveFovLH(45.0f * (3.14f / 180.0f), (float)Width / (float)Height, 0.1f, 1000.0f);
    XMStoreFloat4x4(&cameraProjMat, tmpMat);

    // set starting camera state
    cameraPosition = XMFLOAT4(0.0f, 2.0f, -4.0f, 0.0f);
    cameraTarget = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
    cameraUp = XMFLOAT4(0.0f, 1.0f, 0.0f, 0.0f);

    // build view matrix
    XMVECTOR cPos = XMLoadFloat4(&cameraPosition);
    XMVECTOR cTarg = XMLoadFloat4(&cameraTarget);
    XMVECTOR cUp = XMLoadFloat4(&cameraUp);
    tmpMat = XMMatrixLookAtLH(cPos, cTarg, cUp);
    XMStoreFloat4x4(&cameraViewMat, tmpMat);

    // set starting cubes position
    // first cube
    cube1Position = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f); // set cube 1's position
    XMVECTOR posVec = XMLoadFloat4(&cube1Position); // create xmvector for cube1's position

    tmpMat = XMMatrixTranslationFromVector(posVec); // create translation matrix from cube1's position vector
    XMStoreFloat4x4(&cube1RotMat, XMMatrixIdentity()); // initialize cube1's rotation matrix to identity matrix
    XMStoreFloat4x4(&cube1WorldMat, tmpMat); // store cube1's world matrix

    //// second cube
    //cube2PositionOffset = XMFLOAT4(1.5f, 0.0f, 0.0f, 0.0f);
    //posVec = XMLoadFloat4(&cube2PositionOffset) + XMLoadFloat4(&cube1Position); // create xmvector for cube2's position
                                                                                // we are rotating around cube1 here, so add cube2's position to cube1

    tmpMat = XMMatrixTranslationFromVector(posVec); // create translation matrix from cube2's position offset vector
    XMStoreFloat4x4(&cube2RotMat, XMMatrixIdentity()); // initialize cube2's rotation matrix to identity matrix
    XMStoreFloat4x4(&cube2WorldMat, tmpMat); // store cube2's world matrix

    return true;
}

void Update()
{
    // update app logic, such as moving the camera or figuring out what objects are in view

    // create rotation matrices
    XMMATRIX rotXMat = XMMatrixRotationX(0.0001f);
    XMMATRIX rotYMat = XMMatrixRotationY(0.0002f);
    XMMATRIX rotZMat = XMMatrixRotationZ(0.0003f);

    // add rotation to cube1's rotation matrix and store it
    XMMATRIX rotMat = XMLoadFloat4x4(&cube1RotMat) * rotXMat * rotYMat * rotZMat;
    XMStoreFloat4x4(&cube1RotMat, rotMat);

    // create translation matrix for cube 1 from cube 1's position vector
    XMMATRIX translationMat = XMMatrixTranslationFromVector(XMLoadFloat4(&cube1Position));

    // create cube1's world matrix by first rotating the cube, then positioning the rotated cube
    XMMATRIX worldMat = rotMat * translationMat;

    // store cube1's world matrix
    XMStoreFloat4x4(&cube1WorldMat, worldMat);

    // update constant buffer for cube1
    // create the wvp matrix and store in constant buffer
    XMMATRIX viewMat = XMLoadFloat4x4(&cameraViewMat); // load view matrix
    XMMATRIX projMat = XMLoadFloat4x4(&cameraProjMat); // load projection matrix
    XMMATRIX wvpMat = XMLoadFloat4x4(&cube1WorldMat) * viewMat * projMat; // create wvp matrix
    XMMATRIX transposed = XMMatrixTranspose(wvpMat); // must transpose wvp matrix for the gpu
    XMStoreFloat4x4(&cbPerObject.wvpMat, transposed); // store transposed wvp matrix in constant buffer

    // copy our ConstantBuffer instance to the mapped constant buffer resource
    memcpy(cbvGPUAddress[frameIndex], &cbPerObject, sizeof(cbPerObject));

    // now do cube2's world matrix
    // create rotation matrices for cube2
    rotXMat = XMMatrixRotationX(0.0003f);
    rotYMat = XMMatrixRotationY(0.0002f);
    rotZMat = XMMatrixRotationZ(0.0001f);

    // add rotation to cube2's rotation matrix and store it
    rotMat = rotZMat * (XMLoadFloat4x4(&cube2RotMat) * (rotXMat * rotYMat));
    XMStoreFloat4x4(&cube2RotMat, rotMat);

    // create translation matrix for cube 2 to offset it from cube 1 (its position relative to cube1
    XMMATRIX translationOffsetMat = XMMatrixTranslationFromVector(XMLoadFloat4(&cube2PositionOffset));

    // we want cube 2 to be half the size of cube 1, so we scale it by .5 in all dimensions
    XMMATRIX scaleMat = XMMatrixScaling(0.5f, 0.5f, 0.5f);

    // reuse worldMat. 
    // first we scale cube2. scaling happens relative to point 0,0,0, so you will almost always want to scale first
    // then we translate it. 
    // then we rotate it. rotation always rotates around point 0,0,0
    // finally we move it to cube 1's position, which will cause it to rotate around cube 1
    worldMat = scaleMat * translationOffsetMat * rotMat * translationMat;

    wvpMat = XMLoadFloat4x4(&cube2WorldMat) * viewMat * projMat; // create wvp matrix
    transposed = XMMatrixTranspose(wvpMat); // must transpose wvp matrix for the gpu
    XMStoreFloat4x4(&cbPerObject.wvpMat, transposed); // store transposed wvp matrix in constant buffer

    // copy our ConstantBuffer instance to the mapped constant buffer resource
    memcpy(cbvGPUAddress[frameIndex] + ConstantBufferPerObjectAlignedSize, &cbPerObject, sizeof(cbPerObject));

    // store cube2's world matrix
    XMStoreFloat4x4(&cube2WorldMat, worldMat);
}

CD3DX12_CPU_DESCRIPTOR_HANDLE UpdatePipeline()
{
    HRESULT hr;

    // We have to wait for the gpu to finish with the command allocator before we reset it
    WaitForPreviousFrame();

    // we can only reset an allocator once the gpu is done with it
    // resetting an allocator frees the memory that the command list was stored in
    hr = commandAllocator[frameIndex]->Reset();
    if (FAILED(hr))
    {
        Running = false;
    }

    // reset the command list. by resetting the command list we are putting it into
    // a recording state so we can start recording commands into the command allocator.
    // the command allocator that we reference here may have multiple command lists
    // associated with it, but only one can be recording at any time. Make sure
    // that any other command lists associated to this command allocator are in
    // the closed state (not recording).
    // Here you will pass an initial pipeline state object as the second parameter,
    // but in this tutorial we are only clearing the rtv, and do not actually need
    // anything but an initial default pipeline, which is what we get by setting
    // the second parameter to NULL
    hr = commandList->Reset(commandAllocator[frameIndex], pipelineStateObject);
    if (FAILED(hr))
    {
        Running = false;
    }

    // here we start recording commands into the commandList (which all the commands will be stored in the commandAllocator)

    // transition the "frameIndex" render target from the present state to the render target state so the command list draws to it starting from here
    const auto render_target_state_resource_barrier = CD3DX12_RESOURCE_BARRIER::Transition(renderTargets[frameIndex], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
    commandList->ResourceBarrier(1, &render_target_state_resource_barrier);

    // here we again get the handle to our current render target view so we can set it as the render target in the output merger stage of the pipeline
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), frameIndex, rtvDescriptorSize);
    //copy from this handle into another spot at the end of this function
    //copy to another texture where I'm doing inference
    //or pass this model directly to inference and update model to take n,h,w,c

    // get a handle to the depth/stencil buffer
    CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(dsDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

    // set the render target for the output merger stage (the output of the pipeline)
    commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

    // Clear the render target by using the ClearRenderTargetView command
    const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
    commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

    // clear the depth/stencil buffer
    commandList->ClearDepthStencilView(dsDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    // set root signature
    commandList->SetGraphicsRootSignature(rootSignature); // set the root signature

    // set the descriptor heap
    ID3D12DescriptorHeap* descriptorHeaps[] = { mainDescriptorHeap };
    commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    // set the descriptor table to the descriptor heap (parameter 1, as constant buffer root descriptor is parameter index 0)
    commandList->SetGraphicsRootDescriptorTable(1, mainDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

    commandList->RSSetViewports(1, &viewport); // set the viewports
    commandList->RSSetScissorRects(1, &scissorRect); // set the scissor rects
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST); // set the primitive topology
    commandList->IASetVertexBuffers(0, 1, &vertexBufferView); // set the vertex buffer (using the vertex buffer view)
    commandList->IASetIndexBuffer(&indexBufferView);

    // first cube

    // set cube1's constant buffer
    commandList->SetGraphicsRootConstantBufferView(0, constantBufferUploadHeaps[frameIndex]->GetGPUVirtualAddress());
    //d3dResource = constantBufferUploadHeaps[frameIndex];

    // draw first cube
    commandList->DrawIndexedInstanced(numCubeIndices, 1, 0, 0, 0);

    // second cube

    // set cube2's constant buffer. You can see we are adding the size of ConstantBufferPerObject to the constant buffer
    // resource heaps address. This is because cube1's constant buffer is stored at the beginning of the resource heap, while
    // cube2's constant buffer data is stored after (256 bits from the start of the heap).
    commandList->SetGraphicsRootConstantBufferView(0, constantBufferUploadHeaps[frameIndex]->GetGPUVirtualAddress() + ConstantBufferPerObjectAlignedSize);

    // draw second cube
    commandList->DrawIndexedInstanced(numCubeIndices, 1, 0, 0, 0);

    // transition the "frameIndex" render target from the render target state to the present state. If the debug layer is enabled, you will receive a
    // warning if present is called on the render target when it's not in the present state
    const auto frame_index_resource_barrier = CD3DX12_RESOURCE_BARRIER::Transition(renderTargets[frameIndex], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
    commandList->ResourceBarrier(1, &frame_index_resource_barrier);

    hr = commandList->Close();
    if (FAILED(hr))
    {
        Running = false;
    }
    return rtvHandle;
}

CD3DX12_CPU_DESCRIPTOR_HANDLE Render()
{
    HRESULT hr;

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle = UpdatePipeline(); // update the pipeline by sending commands to the commandqueue

    // create an array of command lists (only one command list here)
    ID3D12CommandList* ppCommandLists[] = { commandList };

    // execute the array of command lists
    commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    // this command goes in at the end of our command queue. we will know when our command queue 
    // has finished because the fence value will be set to "fenceValue" from the GPU since the command
    // queue is being executed on the GPU
    hr = commandQueue->Signal(fence[frameIndex], fenceValue[frameIndex]);
    if (FAILED(hr))
    {
        Running = false;
    }

    // present the current backbuffer
    hr = swapChain->Present(0, 0);
    if (FAILED(hr))
    {
        Running = false;
    }
    return rtvHandle;
}

void Cleanup()
{
    // wait for the gpu to finish all frames
    for (int i = 0; i < frameBufferCount; ++i)
    {
        frameIndex = i;
        WaitForPreviousFrame();
    }

    // get swapchain out of full screen before exiting
    BOOL fs = false;
    if (swapChain->GetFullscreenState(&fs, NULL))
        swapChain->SetFullscreenState(false, NULL);

    SAFE_RELEASE(device);
    SAFE_RELEASE(swapChain);
    SAFE_RELEASE(commandQueue);
    SAFE_RELEASE(rtvDescriptorHeap);
    SAFE_RELEASE(commandList);

    for (int i = 0; i < frameBufferCount; ++i)
    {
        SAFE_RELEASE(renderTargets[i]);
        SAFE_RELEASE(commandAllocator[i]);
        SAFE_RELEASE(fence[i]);
    };

    SAFE_RELEASE(pipelineStateObject);
    SAFE_RELEASE(rootSignature);
    SAFE_RELEASE(vertexBuffer);
    SAFE_RELEASE(indexBuffer);

    SAFE_RELEASE(depthStencilBuffer);
    SAFE_RELEASE(dsDescriptorHeap);

    for (int i = 0; i < frameBufferCount; ++i)
    {
        SAFE_RELEASE(constantBufferUploadHeaps[i]);
    };
}

void WaitForPreviousFrame()
{
    HRESULT hr;

    // swap the current rtv buffer index so we draw on the correct buffer
    frameIndex = swapChain->GetCurrentBackBufferIndex();

    // if the current fence value is still less than "fenceValue", then we know the GPU has not finished executing
    // the command queue since it has not reached the "commandQueue->Signal(fence, fenceValue)" command
    if (fence[frameIndex]->GetCompletedValue() < fenceValue[frameIndex])
    {
        // we have the fence create an event which is signaled once the fence's current value is "fenceValue"
        hr = fence[frameIndex]->SetEventOnCompletion(fenceValue[frameIndex], fenceEvent);
        if (FAILED(hr))
        {
            Running = false;
        }

        // We will wait until the fence has triggered the event that it's current value has reached "fenceValue". once it's value
        // has reached "fenceValue", we know the command queue has finished executing
        WaitForSingleObject(fenceEvent, INFINITE);
    }

    // increment fenceValue for next frame
    fenceValue[frameIndex]++;
}

// get the dxgi format equivilent of a wic format
DXGI_FORMAT GetDXGIFormatFromWICFormat(WICPixelFormatGUID& wicFormatGUID)
{
    if (wicFormatGUID == GUID_WICPixelFormat128bppRGBAFloat) return DXGI_FORMAT_R32G32B32A32_FLOAT;
    else if (wicFormatGUID == GUID_WICPixelFormat64bppRGBAHalf) return DXGI_FORMAT_R16G16B16A16_FLOAT;
    else if (wicFormatGUID == GUID_WICPixelFormat64bppRGBA) return DXGI_FORMAT_R16G16B16A16_UNORM;
    else if (wicFormatGUID == GUID_WICPixelFormat32bppRGBA) return DXGI_FORMAT_R8G8B8A8_UNORM;
    else if (wicFormatGUID == GUID_WICPixelFormat32bppBGRA) return DXGI_FORMAT_B8G8R8A8_UNORM;
    else if (wicFormatGUID == GUID_WICPixelFormat32bppBGR) return DXGI_FORMAT_B8G8R8X8_UNORM;
    else if (wicFormatGUID == GUID_WICPixelFormat32bppRGBA1010102XR) return DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM;

    else if (wicFormatGUID == GUID_WICPixelFormat32bppRGBA1010102) return DXGI_FORMAT_R10G10B10A2_UNORM;
    else if (wicFormatGUID == GUID_WICPixelFormat16bppBGRA5551) return DXGI_FORMAT_B5G5R5A1_UNORM;
    else if (wicFormatGUID == GUID_WICPixelFormat16bppBGR565) return DXGI_FORMAT_B5G6R5_UNORM;
    else if (wicFormatGUID == GUID_WICPixelFormat32bppGrayFloat) return DXGI_FORMAT_R32_FLOAT;
    else if (wicFormatGUID == GUID_WICPixelFormat16bppGrayHalf) return DXGI_FORMAT_R16_FLOAT;
    else if (wicFormatGUID == GUID_WICPixelFormat16bppGray) return DXGI_FORMAT_R16_UNORM;
    else if (wicFormatGUID == GUID_WICPixelFormat8bppGray) return DXGI_FORMAT_R8_UNORM;
    else if (wicFormatGUID == GUID_WICPixelFormat8bppAlpha) return DXGI_FORMAT_A8_UNORM;

    else return DXGI_FORMAT_UNKNOWN;
}

// get a dxgi compatible wic format from another wic format
WICPixelFormatGUID GetConvertToWICFormat(WICPixelFormatGUID& wicFormatGUID)
{
    if (wicFormatGUID == GUID_WICPixelFormatBlackWhite) return GUID_WICPixelFormat8bppGray;
    else if (wicFormatGUID == GUID_WICPixelFormat1bppIndexed) return GUID_WICPixelFormat32bppRGBA;
    else if (wicFormatGUID == GUID_WICPixelFormat2bppIndexed) return GUID_WICPixelFormat32bppRGBA;
    else if (wicFormatGUID == GUID_WICPixelFormat4bppIndexed) return GUID_WICPixelFormat32bppRGBA;
    else if (wicFormatGUID == GUID_WICPixelFormat8bppIndexed) return GUID_WICPixelFormat32bppRGBA;
    else if (wicFormatGUID == GUID_WICPixelFormat2bppGray) return GUID_WICPixelFormat8bppGray;
    else if (wicFormatGUID == GUID_WICPixelFormat4bppGray) return GUID_WICPixelFormat8bppGray;
    else if (wicFormatGUID == GUID_WICPixelFormat16bppGrayFixedPoint) return GUID_WICPixelFormat16bppGrayHalf;
    else if (wicFormatGUID == GUID_WICPixelFormat32bppGrayFixedPoint) return GUID_WICPixelFormat32bppGrayFloat;
    else if (wicFormatGUID == GUID_WICPixelFormat16bppBGR555) return GUID_WICPixelFormat16bppBGRA5551;
    else if (wicFormatGUID == GUID_WICPixelFormat32bppBGR101010) return GUID_WICPixelFormat32bppRGBA1010102;
    else if (wicFormatGUID == GUID_WICPixelFormat24bppBGR) return GUID_WICPixelFormat32bppRGBA;
    else if (wicFormatGUID == GUID_WICPixelFormat24bppRGB) return GUID_WICPixelFormat32bppRGBA;
    else if (wicFormatGUID == GUID_WICPixelFormat32bppPBGRA) return GUID_WICPixelFormat32bppRGBA;
    else if (wicFormatGUID == GUID_WICPixelFormat32bppPRGBA) return GUID_WICPixelFormat32bppRGBA;
    else if (wicFormatGUID == GUID_WICPixelFormat48bppRGB) return GUID_WICPixelFormat64bppRGBA;
    else if (wicFormatGUID == GUID_WICPixelFormat48bppBGR) return GUID_WICPixelFormat64bppRGBA;
    else if (wicFormatGUID == GUID_WICPixelFormat64bppBGRA) return GUID_WICPixelFormat64bppRGBA;
    else if (wicFormatGUID == GUID_WICPixelFormat64bppPRGBA) return GUID_WICPixelFormat64bppRGBA;
    else if (wicFormatGUID == GUID_WICPixelFormat64bppPBGRA) return GUID_WICPixelFormat64bppRGBA;
    else if (wicFormatGUID == GUID_WICPixelFormat48bppRGBFixedPoint) return GUID_WICPixelFormat64bppRGBAHalf;
    else if (wicFormatGUID == GUID_WICPixelFormat48bppBGRFixedPoint) return GUID_WICPixelFormat64bppRGBAHalf;
    else if (wicFormatGUID == GUID_WICPixelFormat64bppRGBAFixedPoint) return GUID_WICPixelFormat64bppRGBAHalf;
    else if (wicFormatGUID == GUID_WICPixelFormat64bppBGRAFixedPoint) return GUID_WICPixelFormat64bppRGBAHalf;
    else if (wicFormatGUID == GUID_WICPixelFormat64bppRGBFixedPoint) return GUID_WICPixelFormat64bppRGBAHalf;
    else if (wicFormatGUID == GUID_WICPixelFormat64bppRGBHalf) return GUID_WICPixelFormat64bppRGBAHalf;
    else if (wicFormatGUID == GUID_WICPixelFormat48bppRGBHalf) return GUID_WICPixelFormat64bppRGBAHalf;
    else if (wicFormatGUID == GUID_WICPixelFormat128bppPRGBAFloat) return GUID_WICPixelFormat128bppRGBAFloat;
    else if (wicFormatGUID == GUID_WICPixelFormat128bppRGBFloat) return GUID_WICPixelFormat128bppRGBAFloat;
    else if (wicFormatGUID == GUID_WICPixelFormat128bppRGBAFixedPoint) return GUID_WICPixelFormat128bppRGBAFloat;
    else if (wicFormatGUID == GUID_WICPixelFormat128bppRGBFixedPoint) return GUID_WICPixelFormat128bppRGBAFloat;
    else if (wicFormatGUID == GUID_WICPixelFormat32bppRGBE) return GUID_WICPixelFormat128bppRGBAFloat;
    else if (wicFormatGUID == GUID_WICPixelFormat32bppCMYK) return GUID_WICPixelFormat32bppRGBA;
    else if (wicFormatGUID == GUID_WICPixelFormat64bppCMYK) return GUID_WICPixelFormat64bppRGBA;
    else if (wicFormatGUID == GUID_WICPixelFormat40bppCMYKAlpha) return GUID_WICPixelFormat64bppRGBA;
    else if (wicFormatGUID == GUID_WICPixelFormat80bppCMYKAlpha) return GUID_WICPixelFormat64bppRGBA;

#if (_WIN32_WINNT >= _WIN32_WINNT_WIN8) || defined(_WIN7_PLATFORM_UPDATE)
    else if (wicFormatGUID == GUID_WICPixelFormat32bppRGB) return GUID_WICPixelFormat32bppRGBA;
    else if (wicFormatGUID == GUID_WICPixelFormat64bppRGB) return GUID_WICPixelFormat64bppRGBA;
    else if (wicFormatGUID == GUID_WICPixelFormat64bppPRGBAHalf) return GUID_WICPixelFormat64bppRGBAHalf;
#endif

    else return GUID_WICPixelFormatDontCare;
}

// get the number of bits per pixel for a dxgi format
int GetDXGIFormatBitsPerPixel(DXGI_FORMAT& dxgiFormat)
{
    if (dxgiFormat == DXGI_FORMAT_R32G32B32A32_FLOAT) return 128;
    else if (dxgiFormat == DXGI_FORMAT_R16G16B16A16_FLOAT) return 64;
    else if (dxgiFormat == DXGI_FORMAT_R16G16B16A16_UNORM) return 64;
    else if (dxgiFormat == DXGI_FORMAT_R8G8B8A8_UNORM) return 32;
    else if (dxgiFormat == DXGI_FORMAT_B8G8R8A8_UNORM) return 32;
    else if (dxgiFormat == DXGI_FORMAT_B8G8R8X8_UNORM) return 32;
    else if (dxgiFormat == DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM) return 32;

    else if (dxgiFormat == DXGI_FORMAT_R10G10B10A2_UNORM) return 32;
    else if (dxgiFormat == DXGI_FORMAT_B5G5R5A1_UNORM) return 16;
    else if (dxgiFormat == DXGI_FORMAT_B5G6R5_UNORM) return 16;
    else if (dxgiFormat == DXGI_FORMAT_R32_FLOAT) return 32;
    else if (dxgiFormat == DXGI_FORMAT_R16_FLOAT) return 16;
    else if (dxgiFormat == DXGI_FORMAT_R16_UNORM) return 16;
    else if (dxgiFormat == DXGI_FORMAT_R8_UNORM) return 8;
    else if (dxgiFormat == DXGI_FORMAT_A8_UNORM) return 8;
}

// load and decode image from file
int LoadImageDataFromFile(BYTE** imageData, D3D12_RESOURCE_DESC& resourceDescription, LPCWSTR filename, int& bytesPerRow)
{
    HRESULT hr;

    // we only need one instance of the imaging factory to create decoders and frames
    static IWICImagingFactory* wicFactory;

    // reset decoder, frame and converter since these will be different for each image we load
    IWICBitmapDecoder* wicDecoder = NULL;
    IWICBitmapFrameDecode* wicFrame = NULL;
    IWICFormatConverter* wicConverter = NULL;

    bool imageConverted = false;

    if (wicFactory == NULL)
    {
        // Initialize the COM library
        CoInitialize(NULL);

        // create the WIC factory
        hr = CoCreateInstance(
            CLSID_WICImagingFactory,
            NULL,
            CLSCTX_INPROC_SERVER,
            IID_PPV_ARGS(&wicFactory)
        );
        if (FAILED(hr)) return 0;
    }

    // load a decoder for the image
    hr = wicFactory->CreateDecoderFromFilename(
        filename,                        // Image we want to load in
        NULL,                            // This is a vendor ID, we do not prefer a specific one so set to null
        GENERIC_READ,                    // We want to read from this file
        WICDecodeMetadataCacheOnLoad,    // We will cache the metadata right away, rather than when needed, which might be unknown
        &wicDecoder                      // the wic decoder to be created
    );
    if (FAILED(hr)) return 0;

    // get image from decoder (this will decode the "frame")
    hr = wicDecoder->GetFrame(0, &wicFrame);
    if (FAILED(hr)) return 0;

    // get wic pixel format of image
    WICPixelFormatGUID pixelFormat;
    hr = wicFrame->GetPixelFormat(&pixelFormat);
    if (FAILED(hr)) return 0;

    // get size of image
    UINT textureWidth, textureHeight;
    hr = wicFrame->GetSize(&textureWidth, &textureHeight);
    if (FAILED(hr)) return 0;

    // we are not handling sRGB types in this tutorial, so if you need that support, you'll have to figure
    // out how to implement the support yourself

    // convert wic pixel format to dxgi pixel format
    DXGI_FORMAT dxgiFormat = GetDXGIFormatFromWICFormat(pixelFormat);

    // if the format of the image is not a supported dxgi format, try to convert it
    if (dxgiFormat == DXGI_FORMAT_UNKNOWN)
    {
        // get a dxgi compatible wic format from the current image format
        WICPixelFormatGUID convertToPixelFormat = GetConvertToWICFormat(pixelFormat);

        // return if no dxgi compatible format was found
        if (convertToPixelFormat == GUID_WICPixelFormatDontCare) return 0;

        // set the dxgi format
        dxgiFormat = GetDXGIFormatFromWICFormat(convertToPixelFormat);

        // create the format converter
        hr = wicFactory->CreateFormatConverter(&wicConverter);
        if (FAILED(hr)) return 0;

        // make sure we can convert to the dxgi compatible format
        BOOL canConvert = FALSE;
        hr = wicConverter->CanConvert(pixelFormat, convertToPixelFormat, &canConvert);
        if (FAILED(hr) || !canConvert) return 0;

        // do the conversion (wicConverter will contain the converted image)
        hr = wicConverter->Initialize(wicFrame, convertToPixelFormat, WICBitmapDitherTypeErrorDiffusion, 0, 0, WICBitmapPaletteTypeCustom);
        if (FAILED(hr)) return 0;

        // this is so we know to get the image data from the wicConverter (otherwise we will get from wicFrame)
        imageConverted = true;
    }

    int bitsPerPixel = GetDXGIFormatBitsPerPixel(dxgiFormat); // number of bits per pixel
    bytesPerRow = (textureWidth * bitsPerPixel) / 8; // number of bytes in each row of the image data
    int imageSize = bytesPerRow * textureHeight; // total image size in bytes

    // allocate enough memory for the raw image data, and set imageData to point to that memory
    *imageData = (BYTE*)malloc(imageSize);

    // copy (decoded) raw image data into the newly allocated memory (imageData)
    if (imageConverted)
    {
        // if image format needed to be converted, the wic converter will contain the converted image
        hr = wicConverter->CopyPixels(0, bytesPerRow, imageSize, *imageData);
        if (FAILED(hr)) return 0;
    }
    else
    {
        // no need to convert, just copy data from the wic frame
        hr = wicFrame->CopyPixels(0, bytesPerRow, imageSize, *imageData);
        if (FAILED(hr)) return 0;
    }

    // now describe the texture with the information we have obtained from the image
    resourceDescription = {};
    resourceDescription.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    resourceDescription.Alignment = 0; // may be 0, 4KB, 64KB, or 4MB. 0 will let runtime decide between 64KB and 4MB (4MB for multi-sampled textures)
    resourceDescription.Width = textureWidth; // width of the texture
    resourceDescription.Height = textureHeight; // height of the texture
    resourceDescription.DepthOrArraySize = 1; // if 3d image, depth of 3d image. Otherwise an array of 1D or 2D textures (we only have one image, so we set 1)
    resourceDescription.MipLevels = 1; // Number of mipmaps. We are not generating mipmaps for this texture, so we have only one level
    resourceDescription.Format = dxgiFormat; // This is the dxgi format of the image (format of the pixels)
    resourceDescription.SampleDesc.Count = 1; // This is the number of samples per pixel, we just want 1 sample
    resourceDescription.SampleDesc.Quality = 0; // The quality level of the samples. Higher is better quality, but worse performance
    resourceDescription.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN; // The arrangement of the pixels. Setting to unknown lets the driver choose the most efficient one
    resourceDescription.Flags = D3D12_RESOURCE_FLAG_NONE; // no flags

    // return the size of the image. remember to delete the image once your done with it (in this tutorial once its uploaded to the gpu)
    return imageSize;
}