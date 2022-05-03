//#include "pch.h"
//
//// dwayne_sanity_test_app.cpp : This file contains the 'main' function. Program execution begins and ends there.
////
//
//#define NOMINMAX
//#define WIN32_LEAN_AND_MEAN
//
//#include <iostream>
//#include <cstdio>
//#include <algorithm>
//#include <numeric>
//#include <functional>
//#include <utility>
//#include <string_view>
//#include <span>
//#include <optional>
//#include <memory>
//
//#include <windows.h>
//#include <d3d12.h>
//#include <wrl/client.h>
////#include "dml_provider_factory.h"
//#include "C:\Users\numform\Windows-Machine-Learning\Samples\WinMLSamplesGallery\packages\Microsoft.ML.OnnxRuntime.DirectML.1.10.0\build\native\include\dml_provider_factory.h"
////#include "onnxruntime_cxx_api.h"
//#include "C:\Users\numform\Windows-Machine-Learning\Samples\WinMLSamplesGallery\packages\Microsoft.ML.OnnxRuntime.DirectML.1.10.0\build\native\include\onnxruntime_cxx_api.h"
//
//////////////////////////////////////////////////////////////////////////////////
//
//#define THROW_IF_FAILED(hr) {HRESULT localHr = (hr); if (FAILED(hr)) throw hr;}
//#define RETURN_IF_FAILED(hr) {HRESULT localHr = (hr); if (FAILED(hr)) return hr;}
//#define THROW_IF_NOT_OK(status) {auto localStatus = (status); if (localStatus) throw E_FAIL;}
//#define RETURN_HR_IF_NOT_OK(status) {auto localStatus = (status); if (localStatus) return E_FAIL;}
//
//template <typename T>
//using BaseType =
//std::remove_cv_t<
//    std::remove_reference_t<
//    std::remove_pointer_t<
//    std::remove_all_extents_t<T>
//    >
//    >
//>;
//
//template<typename T>
//using deleting_unique_ptr = std::unique_ptr<T, std::function<void(T*)>>;
//
//template <typename C, typename T = BaseType<decltype(*std::declval<C>().data())>>
//T GetElementCount(C const& range)
//{
//    return std::accumulate(range.begin(), range.end(), static_cast<T>(1), std::multiplies<T>());
//};
//
//////////////////////////////////////////////////////////////////////////////////
//
//Ort::Value CreateTensorValueUsingD3DResource(
//    ID3D12Device* d3dDevice,
//    OrtDmlApi const& ortDmlApi,
//    Ort::MemoryInfo const& memoryInformation,
//    std::span<const int64_t> dimensions,
//    ONNXTensorElementDataType elementDataType,
//    size_t elementByteSize,
//    /*out*/ void** dmlEpResourceWrapper
//);
//
//////////////////////////////////////////////////////////////////////////////////
//
//int main()
//{
//    // Squeezenet opset v7 https://github.com/onnx/models/blob/master/vision/classification/squeezenet/README.md
//    const wchar_t* modelFilePath = L"./squeezenet1.1-7.onnx";
//    const char* modelInputTensorName = "data";
//    const char* modelOutputTensorName = "squeezenet0_flatten0_reshape0";
//    const std::array<int64_t, 4> inputShape = { 1, 3, 224, 224 };
//    const std::array<int64_t, 2> outputShape = { 1, 1000 };
//
//    const bool passTensorsAsD3DResources = true;
//
//    LARGE_INTEGER startTime;
//    LARGE_INTEGER d3dDeviceCreationTime;
//    LARGE_INTEGER sessionCreationTime;
//    LARGE_INTEGER tensorCreationTime;
//    LARGE_INTEGER bindingTime;
//    LARGE_INTEGER runTime;
//    LARGE_INTEGER synchronizeOutputsTime;
//    LARGE_INTEGER cpuFrequency;
//    QueryPerformanceFrequency(&cpuFrequency);
//    QueryPerformanceCounter(&startTime);
//
//    try
//    {
//        Microsoft::WRL::ComPtr<ID3D12Device> d3d12Device;
//        THROW_IF_FAILED(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&d3d12Device)));
//        QueryPerformanceCounter(&d3dDeviceCreationTime);
//
//        OrtApi const& ortApi = Ort::GetApi(); // Uses ORT_API_VERSION
//        const OrtDmlApi* ortDmlApi;
//        THROW_IF_NOT_OK(ortApi.GetExecutionProviderApi("DML", ORT_API_VERSION, reinterpret_cast<const void**>(&ortDmlApi)));
//
//        // ONNX Runtime setup
//        Ort::Env ortEnvironment(ORT_LOGGING_LEVEL_WARNING, "DirectML_Direct3D_TensorAllocation_Test");
//        Ort::SessionOptions sessionOptions;
//        sessionOptions.SetExecutionMode(ExecutionMode::ORT_SEQUENTIAL);
//        sessionOptions.DisableMemPattern();
//        sessionOptions.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
//        ortApi.AddFreeDimensionOverrideByName(sessionOptions, "batch_size", 1);
//        OrtSessionOptionsAppendExecutionProvider_DML(sessionOptions, 0);
//        Ort::Session session = Ort::Session(ortEnvironment, modelFilePath, sessionOptions);
//        QueryPerformanceCounter(&sessionCreationTime);
//
//        Ort::IoBinding ioBinding = Ort::IoBinding::IoBinding(session);
//        const char* memoryInformationName = passTensorsAsD3DResources ? "DML" : "Cpu";
//        Ort::MemoryInfo memoryInformation(memoryInformationName, OrtAllocatorType::OrtDeviceAllocator, 0, OrtMemType::OrtMemTypeDefault);
//        // Not needed: Ort::Allocator allocator(session, memoryInformation);
//
//        // Create input tensor.
//        Ort::Value inputTensor(nullptr);
//        std::vector<float> inputTensorValues(static_cast<size_t>(GetElementCount(inputShape)), 0.0f);
//        std::iota(inputTensorValues.begin(), inputTensorValues.end(), 0.0f);
//        Microsoft::WRL::ComPtr<IUnknown> inputTensorEpWrapper;
//
//        if (passTensorsAsD3DResources)
//        {
//            // Create empty D3D resource for input.
//            inputTensor = CreateTensorValueUsingD3DResource(
//                d3d12Device.Get(),
//                *ortDmlApi,
//                memoryInformation,
//                inputShape,
//                ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT,
//                sizeof(float),
//                /*out*/ IID_PPV_ARGS_Helper(inputTensorEpWrapper.GetAddressOf())
//            );
//        }
//        else // CPU tensor
//        {
//            inputTensor = Ort::Value::CreateTensor<float>(memoryInformation, inputTensorValues.data(), inputTensorValues.size(), inputShape.data(), 4);
//        }
//
//        // Create output tensor on device memory.
//        Ort::Value outputTensor(nullptr);
//        std::vector<float> outputTensorValues(static_cast<size_t>(GetElementCount(outputShape)), 0.0f);
//        Microsoft::WRL::ComPtr<IUnknown> outputTensorEpWrapper;
//
//        if (passTensorsAsD3DResources)
//        {
//            outputTensor = CreateTensorValueUsingD3DResource(
//                d3d12Device.Get(),
//                *ortDmlApi,
//                memoryInformation,
//                outputShape,
//                ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT,
//                sizeof(float),
//                /*out*/ IID_PPV_ARGS_Helper(outputTensorEpWrapper.GetAddressOf())
//            );
//        }
//        else // CPU tensor
//        {
//            outputTensor = Ort::Value::CreateTensor<float>(memoryInformation, outputTensorValues.data(), outputTensorValues.size(), outputShape.data(), 4);
//        }
//
//        QueryPerformanceCounter(&tensorCreationTime);
//
//        ////////////////////////////////////////
//        // Bind the tensor inputs to the model, and run it.
//        ioBinding.BindInput(modelInputTensorName, inputTensor);
//        ioBinding.BindOutput(modelOutputTensorName, outputTensor);
//        ioBinding.SynchronizeInputs();
//        QueryPerformanceCounter(&bindingTime);
//
//        Ort::RunOptions runOptions;
//
//        // TODO: Upload inputTensorValues to GPU inputTensor.
//
//        printf("Beginning execution.\n");
//        printf("Running Session.\n");
//        session.Run(runOptions, ioBinding);
//        QueryPerformanceCounter(&runTime);
//        printf("Synchronizing outputs.\n");
//        ioBinding.SynchronizeOutputs();
//        QueryPerformanceCounter(&synchronizeOutputsTime);
//        printf("Finished execution.\n");
//
//        auto printDuration = [=](char const* message, LARGE_INTEGER qpcTime) mutable
//        {
//            double durationMs = static_cast<double>(qpcTime.QuadPart - startTime.QuadPart);
//            durationMs /= static_cast<double>(cpuFrequency.QuadPart);
//            durationMs *= 1000.0;
//            printf("%s % 12.6f\n", message, durationMs);
//
//            startTime = qpcTime;
//        };
//        printDuration("d3dDeviceCreationTime ...", d3dDeviceCreationTime);
//        printDuration("sessionCreationTime .....", sessionCreationTime);
//        printDuration("tensorCreationTime ......", tensorCreationTime);
//        printDuration("bindingTime .............", bindingTime);
//        printDuration("runTime .................", runTime);
//        printDuration("synchronizeOutputsTime ..", synchronizeOutputsTime);
//
//        // TODO: Download inputTensorValues from GPU outputTensor.
//
//        ////////////////////////////////////////
//        // Print the top results if the output tensors were on the CPU.
//        if (!passTensorsAsD3DResources)
//        {
//#if 1 // Print first 10 values.
//            int min = 0;
//            if (outputTensorValues.size() < size_t(10))
//                min = outputTensorValues.size();
//            else
//                min = size_t(10);
//            for (int i = 0; i <= min; ++i)
//            {
//                printf("output[%d] = %f\n", i, outputTensorValues[i]);
//            }
//#else // Print top 10.
//            std::vector<uint32_t> indices(outputTensorValues.size(), 0);
//            std::iota(indices.begin(), indices.end(), 0);
//            sort(
//                indices.begin(),
//                indices.end(),
//                [&](uint32_t a, uint32_t b)
//                {
//                    return (outputTensorValues[a] > outputTensorValues[b]);
//                }
//            );
//            for (int i = 0; i <= std::min(indices.size(), size_t(10)); ++i)
//            {
//                printf("output[%d] = %f\n", indices[i], outputTensorValues[indices[i]]);
//            }
//#endif
//        }
//    }
//    catch (Ort::Exception const& exception)
//    {
//        printf("Error running model inference: %s\n", exception.what());
//        return EXIT_FAILURE;
//    }
//    catch (std::exception const& exception)
//    {
//        printf("Error running model inference: %s\n", exception.what());
//        return EXIT_FAILURE;
//    }
//
//    return 0;
//}
//
//Microsoft::WRL::ComPtr<ID3D12Resource> CreateD3D12ResourceForTensor(
//    ID3D12Device* d3dDevice,
//    size_t elementByteSize,
//    std::span<const int64_t> tensorDimensions
//)
//{
//    // Try to allocate the backing memory for the caller
//    auto bufferSize = GetElementCount(tensorDimensions);
//    size_t bufferByteSize = static_cast<size_t>(bufferSize * elementByteSize);
//
//    // DML needs the resources' sizes to be a multiple of 4 bytes
//    (bufferByteSize += 3) &= ~3;
//
//    D3D12_HEAP_PROPERTIES heapProperties = {
//        D3D12_HEAP_TYPE_DEFAULT,
//        D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
//        D3D12_MEMORY_POOL_UNKNOWN,
//        0,
//        0
//    };
//    D3D12_RESOURCE_DESC resourceDesc = {
//        D3D12_RESOURCE_DIMENSION_BUFFER,
//        0,
//        static_cast<uint64_t>(bufferByteSize),
//        1,
//        1,
//        1,
//        DXGI_FORMAT_UNKNOWN,
//        {1, 0},
//        D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
//        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
//    };
//
//    Microsoft::WRL::ComPtr<ID3D12Resource> gpuResource;
//    THROW_IF_FAILED(d3dDevice->CreateCommittedResource(
//        &heapProperties,
//        D3D12_HEAP_FLAG_NONE,
//        &resourceDesc,
//        D3D12_RESOURCE_STATE_COMMON,
//        nullptr,
//        __uuidof(ID3D12Resource),
//        /*out*/ &gpuResource
//    ));
//
//    return gpuResource;
//}
//
//Ort::Value CreateTensorValueFromExistingD3DResource(
//    OrtDmlApi const& ortDmlApi,
//    Ort::MemoryInfo const& memoryInformation,
//    ID3D12Resource* d3dResource,
//    std::span<const int64_t> tensorDimensions,
//    ONNXTensorElementDataType elementDataType,
//    /*out*/ void** dmlEpResourceWrapper // Must stay alive with Ort::Value.
//)
//{
//    *dmlEpResourceWrapper = nullptr;
//
//    void* dmlAllocatorResource;
//    THROW_IF_NOT_OK(ortDmlApi.CreateGPUAllocationFromD3DResource(d3dResource, &dmlAllocatorResource));
//    auto deleter = [&](void*) {ortDmlApi.FreeGPUAllocation(dmlAllocatorResource); };
//    deleting_unique_ptr<void> dmlAllocatorResourceCleanup(dmlAllocatorResource, deleter);
//
//    size_t tensorByteSize = static_cast<size_t>(d3dResource->GetDesc().Width);
//    Ort::Value newValue(
//        Ort::Value::CreateTensor(
//            memoryInformation,
//            dmlAllocatorResource,
//            tensorByteSize,
//            tensorDimensions.data(),
//            tensorDimensions.size(),
//            elementDataType
//        )
//    );
//
//    // Return values and the wrapped resource.
//    // TODO: Is there some way to get Ort::Value to just own the D3DResource
//    // directly so that it gets freed after execution or session destruction?
//    *dmlEpResourceWrapper = dmlAllocatorResource;
//    dmlAllocatorResourceCleanup.release();
//
//    return newValue;
//}
//
//Ort::Value CreateTensorValueUsingD3DResource(
//    ID3D12Device* d3d12Device,
//    OrtDmlApi const& ortDmlApi,
//    Ort::MemoryInfo const& memoryInformation,
//    std::span<const int64_t> tensorDimensions,
//    ONNXTensorElementDataType elementDataType,
//    size_t elementByteSize,
//    /*out*/ void** dmlEpResourceWrapper // Must stay alive with Ort::Value.
//)
//{
//    // Create empty resource (values don't matter because we won't read them back anyway).
//    Microsoft::WRL::ComPtr<ID3D12Resource> d3dResource = CreateD3D12ResourceForTensor(
//        d3d12Device,
//        sizeof(float),
//        tensorDimensions
//    );
//
//    return CreateTensorValueFromExistingD3DResource(
//        ortDmlApi,
//        memoryInformation,
//        d3dResource.Get(),
//        tensorDimensions,
//        ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT,
//        /*out*/ dmlEpResourceWrapper
//    );
//}
#include "pch.h"
//
//// dwayne_sanity_test_app.cpp : This file contains the 'main' function. Program execution begins and ends there.
////
//
//#define NOMINMAX
//#define WIN32_LEAN_AND_MEAN
//
//#include <iostream>
//#include <cstdio>
//#include <algorithm>
//#include <numeric>
//#include <functional>
//#include <utility>
//#include <string_view>
//#include <span>
//#include <optional>
//#include <memory>
//
//#include <windows.h>
//#include <d3d12.h>
//#include <wrl/client.h>
////#include "dml_provider_factory.h"
//#include "C:\Users\numform\Windows-Machine-Learning\Samples\WinMLSamplesGallery\packages\Microsoft.ML.OnnxRuntime.DirectML.1.10.0\build\native\include\dml_provider_factory.h"
////#include "onnxruntime_cxx_api.h"
//#include "C:\Users\numform\Windows-Machine-Learning\Samples\WinMLSamplesGallery\packages\Microsoft.ML.OnnxRuntime.DirectML.1.10.0\build\native\include\onnxruntime_cxx_api.h"
//
//////////////////////////////////////////////////////////////////////////////////
//
//#define THROW_IF_FAILED(hr) {HRESULT localHr = (hr); if (FAILED(hr)) throw hr;}
//#define RETURN_IF_FAILED(hr) {HRESULT localHr = (hr); if (FAILED(hr)) return hr;}
//#define THROW_IF_NOT_OK(status) {auto localStatus = (status); if (localStatus) throw E_FAIL;}
//#define RETURN_HR_IF_NOT_OK(status) {auto localStatus = (status); if (localStatus) return E_FAIL;}
//
//template <typename T>
//using BaseType =
//std::remove_cv_t<
//    std::remove_reference_t<
//    std::remove_pointer_t<
//    std::remove_all_extents_t<T>
//    >
//    >
//>;
//
//template<typename T>
//using deleting_unique_ptr = std::unique_ptr<T, std::function<void(T*)>>;
//
//template <typename C, typename T = BaseType<decltype(*std::declval<C>().data())>>
//T GetElementCount(C const& range)
//{
//    return std::accumulate(range.begin(), range.end(), static_cast<T>(1), std::multiplies<T>());
//};
//
//////////////////////////////////////////////////////////////////////////////////
//
//Ort::Value CreateTensorValueUsingD3DResource(
//    ID3D12Device* d3dDevice,
//    OrtDmlApi const& ortDmlApi,
//    Ort::MemoryInfo const& memoryInformation,
//    std::span<const int64_t> dimensions,
//    ONNXTensorElementDataType elementDataType,
//    size_t elementByteSize,
//    /*out*/ void** dmlEpResourceWrapper
//);
//
//////////////////////////////////////////////////////////////////////////////////
//
//int main()
//{
//    // Squeezenet opset v7 https://github.com/onnx/models/blob/master/vision/classification/squeezenet/README.md
//    const wchar_t* modelFilePath = L"./squeezenet1.1-7.onnx";
//    const char* modelInputTensorName = "data";
//    const char* modelOutputTensorName = "squeezenet0_flatten0_reshape0";
//    const std::array<int64_t, 4> inputShape = { 1, 3, 224, 224 };
//    const std::array<int64_t, 2> outputShape = { 1, 1000 };
//
//    const bool passTensorsAsD3DResources = true;
//
//    LARGE_INTEGER startTime;
//    LARGE_INTEGER d3dDeviceCreationTime;
//    LARGE_INTEGER sessionCreationTime;
//    LARGE_INTEGER tensorCreationTime;
//    LARGE_INTEGER bindingTime;
//    LARGE_INTEGER runTime;
//    LARGE_INTEGER synchronizeOutputsTime;
//    LARGE_INTEGER cpuFrequency;
//    QueryPerformanceFrequency(&cpuFrequency);
//    QueryPerformanceCounter(&startTime);
//
//    try
//    {
//        Microsoft::WRL::ComPtr<ID3D12Device> d3d12Device;
//        THROW_IF_FAILED(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&d3d12Device)));
//        QueryPerformanceCounter(&d3dDeviceCreationTime);
//
//        OrtApi const& ortApi = Ort::GetApi(); // Uses ORT_API_VERSION
//        const OrtDmlApi* ortDmlApi;
//        THROW_IF_NOT_OK(ortApi.GetExecutionProviderApi("DML", ORT_API_VERSION, reinterpret_cast<const void**>(&ortDmlApi)));
//
//        // ONNX Runtime setup
//        Ort::Env ortEnvironment(ORT_LOGGING_LEVEL_WARNING, "DirectML_Direct3D_TensorAllocation_Test");
//        Ort::SessionOptions sessionOptions;
//        sessionOptions.SetExecutionMode(ExecutionMode::ORT_SEQUENTIAL);
//        sessionOptions.DisableMemPattern();
//        sessionOptions.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
//        ortApi.AddFreeDimensionOverrideByName(sessionOptions, "batch_size", 1);
//        OrtSessionOptionsAppendExecutionProvider_DML(sessionOptions, 0);
//        Ort::Session session = Ort::Session(ortEnvironment, modelFilePath, sessionOptions);
//        QueryPerformanceCounter(&sessionCreationTime);
//
//        Ort::IoBinding ioBinding = Ort::IoBinding::IoBinding(session);
//        const char* memoryInformationName = passTensorsAsD3DResources ? "DML" : "Cpu";
//        Ort::MemoryInfo memoryInformation(memoryInformationName, OrtAllocatorType::OrtDeviceAllocator, 0, OrtMemType::OrtMemTypeDefault);
//        // Not needed: Ort::Allocator allocator(session, memoryInformation);
//
//        // Create input tensor.
//        Ort::Value inputTensor(nullptr);
//        std::vector<float> inputTensorValues(static_cast<size_t>(GetElementCount(inputShape)), 0.0f);
//        std::iota(inputTensorValues.begin(), inputTensorValues.end(), 0.0f);
//        Microsoft::WRL::ComPtr<IUnknown> inputTensorEpWrapper;
//
//        if (passTensorsAsD3DResources)
//        {
//            // Create empty D3D resource for input.
//            inputTensor = CreateTensorValueUsingD3DResource(
//                d3d12Device.Get(),
//                *ortDmlApi,
//                memoryInformation,
//                inputShape,
//                ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT,
//                sizeof(float),
//                /*out*/ IID_PPV_ARGS_Helper(inputTensorEpWrapper.GetAddressOf())
//            );
//        }
//        else // CPU tensor
//        {
//            inputTensor = Ort::Value::CreateTensor<float>(memoryInformation, inputTensorValues.data(), inputTensorValues.size(), inputShape.data(), 4);
//        }
//
//        // Create output tensor on device memory.
//        Ort::Value outputTensor(nullptr);
//        std::vector<float> outputTensorValues(static_cast<size_t>(GetElementCount(outputShape)), 0.0f);
//        Microsoft::WRL::ComPtr<IUnknown> outputTensorEpWrapper;
//
//        if (passTensorsAsD3DResources)
//        {
//            outputTensor = CreateTensorValueUsingD3DResource(
//                d3d12Device.Get(),
//                *ortDmlApi,
//                memoryInformation,
//                outputShape,
//                ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT,
//                sizeof(float),
//                /*out*/ IID_PPV_ARGS_Helper(outputTensorEpWrapper.GetAddressOf())
//            );
//        }
//        else // CPU tensor
//        {
//            outputTensor = Ort::Value::CreateTensor<float>(memoryInformation, outputTensorValues.data(), outputTensorValues.size(), outputShape.data(), 4);
//        }
//
//        QueryPerformanceCounter(&tensorCreationTime);
//
//        ////////////////////////////////////////
//        // Bind the tensor inputs to the model, and run it.
//        ioBinding.BindInput(modelInputTensorName, inputTensor);
//        ioBinding.BindOutput(modelOutputTensorName, outputTensor);
//        ioBinding.SynchronizeInputs();
//        QueryPerformanceCounter(&bindingTime);
//
//        Ort::RunOptions runOptions;
//
//        // TODO: Upload inputTensorValues to GPU inputTensor.
//
//        printf("Beginning execution.\n");
//        printf("Running Session.\n");
//        session.Run(runOptions, ioBinding);
//        QueryPerformanceCounter(&runTime);
//        printf("Synchronizing outputs.\n");
//        ioBinding.SynchronizeOutputs();
//        QueryPerformanceCounter(&synchronizeOutputsTime);
//        printf("Finished execution.\n");
//
//        auto printDuration = [=](char const* message, LARGE_INTEGER qpcTime) mutable
//        {
//            double durationMs = static_cast<double>(qpcTime.QuadPart - startTime.QuadPart);
//            durationMs /= static_cast<double>(cpuFrequency.QuadPart);
//            durationMs *= 1000.0;
//            printf("%s % 12.6f\n", message, durationMs);
//
//            startTime = qpcTime;
//        };
//        printDuration("d3dDeviceCreationTime ...", d3dDeviceCreationTime);
//        printDuration("sessionCreationTime .....", sessionCreationTime);
//        printDuration("tensorCreationTime ......", tensorCreationTime);
//        printDuration("bindingTime .............", bindingTime);
//        printDuration("runTime .................", runTime);
//        printDuration("synchronizeOutputsTime ..", synchronizeOutputsTime);
//
//        // TODO: Download inputTensorValues from GPU outputTensor.
//
//        ////////////////////////////////////////
//        // Print the top results if the output tensors were on the CPU.
//        if (!passTensorsAsD3DResources)
//        {
//#if 1 // Print first 10 values.
//            int min = 0;
//            if (outputTensorValues.size() < size_t(10))
//                min = outputTensorValues.size();
//            else
//                min = size_t(10);
//            for (int i = 0; i <= min; ++i)
//            {
//                printf("output[%d] = %f\n", i, outputTensorValues[i]);
//            }
//#else // Print top 10.
//            std::vector<uint32_t> indices(outputTensorValues.size(), 0);
//            std::iota(indices.begin(), indices.end(), 0);
//            sort(
//                indices.begin(),
//                indices.end(),
//                [&](uint32_t a, uint32_t b)
//                {
//                    return (outputTensorValues[a] > outputTensorValues[b]);
//                }
//            );
//            for (int i = 0; i <= std::min(indices.size(), size_t(10)); ++i)
//            {
//                printf("output[%d] = %f\n", indices[i], outputTensorValues[indices[i]]);
//            }
//#endif
//        }
//    }
//    catch (Ort::Exception const& exception)
//    {
//        printf("Error running model inference: %s\n", exception.what());
//        return EXIT_FAILURE;
//    }
//    catch (std::exception const& exception)
//    {
//        printf("Error running model inference: %s\n", exception.what());
//        return EXIT_FAILURE;
//    }
//
//    return 0;
//}
//
//Microsoft::WRL::ComPtr<ID3D12Resource> CreateD3D12ResourceForTensor(
//    ID3D12Device* d3dDevice,
//    size_t elementByteSize,
//    std::span<const int64_t> tensorDimensions
//)
//{
//    // Try to allocate the backing memory for the caller
//    auto bufferSize = GetElementCount(tensorDimensions);
//    size_t bufferByteSize = static_cast<size_t>(bufferSize * elementByteSize);
//
//    // DML needs the resources' sizes to be a multiple of 4 bytes
//    (bufferByteSize += 3) &= ~3;
//
//    D3D12_HEAP_PROPERTIES heapProperties = {
//        D3D12_HEAP_TYPE_DEFAULT,
//        D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
//        D3D12_MEMORY_POOL_UNKNOWN,
//        0,
//        0
//    };
//    D3D12_RESOURCE_DESC resourceDesc = {
//        D3D12_RESOURCE_DIMENSION_BUFFER,
//        0,
//        static_cast<uint64_t>(bufferByteSize),
//        1,
//        1,
//        1,
//        DXGI_FORMAT_UNKNOWN,
//        {1, 0},
//        D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
//        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
//    };
//
//    Microsoft::WRL::ComPtr<ID3D12Resource> gpuResource;
//    THROW_IF_FAILED(d3dDevice->CreateCommittedResource(
//        &heapProperties,
//        D3D12_HEAP_FLAG_NONE,
//        &resourceDesc,
//        D3D12_RESOURCE_STATE_COMMON,
//        nullptr,
//        __uuidof(ID3D12Resource),
//        /*out*/ &gpuResource
//    ));
//
//    return gpuResource;
//}
//
//Ort::Value CreateTensorValueFromExistingD3DResource(
//    OrtDmlApi const& ortDmlApi,
//    Ort::MemoryInfo const& memoryInformation,
//    ID3D12Resource* d3dResource,
//    std::span<const int64_t> tensorDimensions,
//    ONNXTensorElementDataType elementDataType,
//    /*out*/ void** dmlEpResourceWrapper // Must stay alive with Ort::Value.
//)
//{
//    *dmlEpResourceWrapper = nullptr;
//
//    void* dmlAllocatorResource;
//    THROW_IF_NOT_OK(ortDmlApi.CreateGPUAllocationFromD3DResource(d3dResource, &dmlAllocatorResource));
//    auto deleter = [&](void*) {ortDmlApi.FreeGPUAllocation(dmlAllocatorResource); };
//    deleting_unique_ptr<void> dmlAllocatorResourceCleanup(dmlAllocatorResource, deleter);
//
//    size_t tensorByteSize = static_cast<size_t>(d3dResource->GetDesc().Width);
//    Ort::Value newValue(
//        Ort::Value::CreateTensor(
//            memoryInformation,
//            dmlAllocatorResource,
//            tensorByteSize,
//            tensorDimensions.data(),
//            tensorDimensions.size(),
//            elementDataType
//        )
//    );
//
//    // Return values and the wrapped resource.
//    // TODO: Is there some way to get Ort::Value to just own the D3DResource
//    // directly so that it gets freed after execution or session destruction?
//    *dmlEpResourceWrapper = dmlAllocatorResource;
//    dmlAllocatorResourceCleanup.release();
//
//    return newValue;
//}
//
//Ort::Value CreateTensorValueUsingD3DResource(
//    ID3D12Device* d3d12Device,
//    OrtDmlApi const& ortDmlApi,
//    Ort::MemoryInfo const& memoryInformation,
//    std::span<const int64_t> tensorDimensions,
//    ONNXTensorElementDataType elementDataType,
//    size_t elementByteSize,
//    /*out*/ void** dmlEpResourceWrapper // Must stay alive with Ort::Value.
//)
//{
//    // Create empty resource (values don't matter because we won't read them back anyway).
//    Microsoft::WRL::ComPtr<ID3D12Resource> d3dResource = CreateD3D12ResourceForTensor(
//        d3d12Device,
//        sizeof(float),
//        tensorDimensions
//    );
//
//    return CreateTensorValueFromExistingD3DResource(
//        ortDmlApi,
//        memoryInformation,
//        d3dResource.Get(),
//        tensorDimensions,
//        ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT,
//        /*out*/ dmlEpResourceWrapper
//    );
//}
