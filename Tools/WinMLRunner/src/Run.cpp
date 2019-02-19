#include "Common.h"
#include "OutputHelper.h"
#include "ModelBinding.h"
#include "BindingUtilities.h"
#include <filesystem>
#include <d3d11.h>
#include <Windows.Graphics.DirectX.Direct3D11.interop.h>
#include "Run.h"
#include "Scenarios.h"
#include <initguid.h>
#include <dxcore.h>

#define THROW_IF_FAILED(hr) { if (FAILED(hr)) throw hresult_error(hr); }

using namespace winrt::Windows::Graphics::DirectX::Direct3D11;

LearningModel LoadModel(const std::wstring &path, bool capturePerf, OutputHelper& output, const CommandLineArgs& args, uint32_t iterationNum, Profiler<WINML_MODEL_TEST_PERF>& profiler)
{
    LearningModel model = nullptr;
    output.PrintLoadingInfo(path);

    try
    {
        if (capturePerf)
        {
            WINML_PROFILING_START(profiler, WINML_MODEL_TEST_PERF::LOAD_MODEL);
        }
        model = LearningModel::LoadFromFilePath(path);

        if (capturePerf)
        {
            WINML_PROFILING_STOP(profiler, WINML_MODEL_TEST_PERF::LOAD_MODEL);
            if (args.IsPerIterationCapture())
            {
                output.SaveLoadTimes(profiler, iterationNum);
            }
        }
    }
    catch (hresult_error hr)
    {
        std::wcout << "Load Model: " << path << " [FAILED]" << std::endl;
        std::wcout << hr.message().c_str() << std::endl;
        throw;
    }

    output.PrintModelInfo(path, model);

    return model;
}

std::vector<std::wstring> GetModelsInDirectory(CommandLineArgs& args, OutputHelper* output)
{
    std::vector<std::wstring> modelPaths;

    std::wstring folderPath = args.FolderPath();
    for (auto & it : std::filesystem::directory_iterator(args.FolderPath()))
    {
        std::string path = it.path().string();

        if (it.path().string().find(".onnx") != std::string::npos ||
            it.path().string().find(".pb") != std::string::npos)
        {
            std::wstring fileName;
            fileName.assign(path.begin(), path.end());
            args.SetModelPath(fileName);
            modelPaths.push_back(fileName);
        }
    }

    return modelPaths;
}

std::vector<ILearningModelFeatureValue> GenerateInputFeatures(
    const LearningModel& model,
    const CommandLineArgs& args,
    InputBindingType inputBindingType,
    InputDataType inputDataType,
    const IDirect3DDevice winrtDevice,
    uint32_t iterationNum)
{
    std::vector<ILearningModelFeatureValue> inputFeatures;

    for (uint32_t i = 0; i < model.InputFeatures().Size(); i++)
    {
        auto&& description = model.InputFeatures().GetAt(i);

        if (inputDataType == InputDataType::Tensor || i > 0)
        {
            // For now, only the first input can be bound with real data
            auto tensorFeature = BindingUtilities::CreateBindableTensor(description, args);
            inputFeatures.push_back(tensorFeature);
        }
        else
        {
            auto imageFeature = BindingUtilities::CreateBindableImage(description, args.ImagePath(), inputBindingType, inputDataType, winrtDevice, args, iterationNum);
            inputFeatures.push_back(imageFeature);
        }
    }

    return inputFeatures;
}

HRESULT BindInputFeatures(const LearningModel& model, const LearningModelBinding& context, const std::vector<ILearningModelFeatureValue>& inputFeatures, const CommandLineArgs& args, OutputHelper& output, bool capturePerf, uint32_t iterationNum, Profiler<WINML_MODEL_TEST_PERF>& profiler)
{
    assert(model.InputFeatures().Size() == inputFeatures.size());

    try
    {
        context.Clear();

        if (capturePerf)
        {
            WINML_PROFILING_START(profiler, iterationNum == 0 ? WINML_MODEL_TEST_PERF::BIND_VALUE_FIRST_RUN : WINML_MODEL_TEST_PERF::BIND_VALUE);
        }

        for (uint32_t i = 0; i < model.InputFeatures().Size(); i++)
        {
            auto&& description = model.InputFeatures().GetAt(i);
            context.Bind(description.Name(), inputFeatures[i]);
        }

        if (capturePerf)
        {
            WINML_PROFILING_STOP(profiler, iterationNum == 0 ? WINML_MODEL_TEST_PERF::BIND_VALUE_FIRST_RUN : WINML_MODEL_TEST_PERF::BIND_VALUE);
            if (args.IsPerIterationCapture())
            {
                output.SaveBindTimes(profiler, iterationNum);
            }
        }
    }
    catch (hresult_error hr)
    {
        std::cout << "[FAILED] Could Not Bind Input To Context" << std::endl;
        std::wcout << hr.message().c_str() << std::endl;
        return hr.code();
    }

    return S_OK;
}

HRESULT EvaluateModel(
    LearningModelEvaluationResult& result,
    const LearningModel& model,
    const LearningModelBinding& context,
    LearningModelSession& session,
    const CommandLineArgs& args,
    OutputHelper& output,
    bool capturePerf,
    uint32_t iterationNum,
    Profiler<WINML_MODEL_TEST_PERF>& profiler
)
{
    try
    {
        if (capturePerf)
        {
            WINML_PROFILING_START(profiler, iterationNum == 0 ? WINML_MODEL_TEST_PERF::EVAL_MODEL_FIRST_RUN : WINML_MODEL_TEST_PERF::EVAL_MODEL);
        }

        result = session.Evaluate(context, L"");

        if (capturePerf)
        {
            WINML_PROFILING_STOP(profiler, iterationNum == 0 ? WINML_MODEL_TEST_PERF::EVAL_MODEL_FIRST_RUN : WINML_MODEL_TEST_PERF::EVAL_MODEL);
            if (args.IsPerIterationCapture())
            {
                output.SaveEvalPerformance(profiler, iterationNum);
            }
        }
    }
    catch (winrt::hresult_error hr)
    {
        std::cout << "[FAILED]" << std::endl;
        std::wcout << hr.message().c_str() << std::endl;
        return hr.code();
    }

    if (args.IsSaveTensor())
    {
        BindingUtilities::SaveEvaluationResults(model, args, result.Outputs(), output, iterationNum + 1);
    }
    return S_OK;
}

// Binds and evaluates the user-specified model and outputs success/failure for each step. If the
// perf flag is used, it will output the CPU, GPU, and wall-clock time for each step to the
// command-line and to a CSV file.
HRESULT EvaluateModel(
    const LearningModel& model,
    const CommandLineArgs& args,
    OutputHelper& output,
    DeviceType deviceType,
    InputBindingType inputBindingType,
    InputDataType inputDataType,
    DeviceCreationLocation deviceCreationLocation,
    Profiler<WINML_MODEL_TEST_PERF>& profiler
)
{
    if (model == nullptr)
    {
        return hresult_invalid_argument().code();
    }

    LearningModelSession session = nullptr;
    IDirect3DDevice winrtDevice = nullptr;
    com_ptr<::IUnknown> spUnkLearningModelDevice;

    try
    {
        UINT adapterIndex = args.GetGPUAdapterIndex();

        if (deviceCreationLocation == DeviceCreationLocation::ClientCode && deviceType != DeviceType::CPU)
        {
            // Enumerate Adapters to pick the requested one.
            com_ptr<IDXGIFactory6> factory;
            HRESULT hr = CreateDXGIFactory(__uuidof(IDXGIFactory6), factory.put_void());
            THROW_IF_FAILED(hr);

            com_ptr<IDXGIAdapter> adapter;
            switch (deviceType)
            {
            case DeviceType::DefaultGPU:
                hr = factory->EnumAdapterByGpuPreference(0, DXGI_GPU_PREFERENCE_UNSPECIFIED, __uuidof(IDXGIAdapter), adapter.put_void());
                break;
            case DeviceType::MinPowerGPU:
                hr = factory->EnumAdapterByGpuPreference(0, DXGI_GPU_PREFERENCE_MINIMUM_POWER, __uuidof(IDXGIAdapter), adapter.put_void());
                break;
            case DeviceType::HighPerfGPU:
                hr = factory->EnumAdapterByGpuPreference(0, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, __uuidof(IDXGIAdapter), adapter.put_void());
                break;
            default:
                throw hresult(E_INVALIDARG);
            }
            THROW_IF_FAILED(hr);

            // Creating the device on the client and using it to create the video frame and initialize the session makes sure that everything is on
            // the same device. This usually avoids an expensive cross-device and cross-videoframe copy via the VideoFrame pipeline.
            com_ptr<ID3D11Device> d3d11Device;
            hr = D3D11CreateDevice(adapter.get(), D3D_DRIVER_TYPE_UNKNOWN, nullptr, D3D11_CREATE_DEVICE_BGRA_SUPPORT, nullptr, 0, D3D11_SDK_VERSION, d3d11Device.put(), nullptr, nullptr);
            THROW_IF_FAILED(hr);

            com_ptr<IDXGIDevice> dxgiDevice;
            hr = d3d11Device->QueryInterface(__uuidof(IDXGIDevice), dxgiDevice.put_void());
            THROW_IF_FAILED(hr);

            com_ptr<IInspectable> inspectableDevice;
            hr = CreateDirect3D11DeviceFromDXGIDevice(dxgiDevice.get(), inspectableDevice.put());
            THROW_IF_FAILED(hr);

            winrtDevice = inspectableDevice.as<IDirect3DDevice>();
            LearningModelDevice learningModelDevice = LearningModelDevice::CreateFromDirect3D11Device(winrtDevice);
            output.PrintLearningModelDevice(deviceType, learningModelDevice);
            if (args.IsPerformanceCapture())
            {
                WINML_PROFILING_START(profiler, WINML_MODEL_TEST_PERF::CREATE_SESSION);
            }
            session = LearningModelSession(model, learningModelDevice);
            if (args.IsPerformanceCapture())
            {
                WINML_PROFILING_STOP(profiler, WINML_MODEL_TEST_PERF::CREATE_SESSION);
            }
        }
        else if ((TypeHelper::GetWinmlDeviceKind(deviceType) != LearningModelDeviceKind::Cpu) && (adapterIndex != -1))
        {
            HRESULT hr = S_OK;
            IUnknown* pAdapter = NULL;
            D3D_FEATURE_LEVEL d3dFeatureLevel = D3D_FEATURE_LEVEL::D3D_FEATURE_LEVEL_1_0_CORE;
            D3D12_COMMAND_LIST_TYPE commandQueueType = D3D12_COMMAND_LIST_TYPE_COMPUTE;

            com_ptr<IDXCoreAdapterFactory> spFactory;

            hr = DXCoreCreateAdapterFactory(__uuidof(IDXCoreAdapterFactory), spFactory.put_void());
            THROW_IF_FAILED(hr);

            com_ptr<IDXCoreAdapterList> spAdapterList;

            const GUID dxGUIDs[] = { DXCORE_ADAPTER_ATTRIBUTE_D3D12_CORE_COMPUTE };

            hr = spFactory->GetAdapterList(dxGUIDs, ARRAYSIZE(dxGUIDs), spAdapterList.put());
            THROW_IF_FAILED(hr);

            com_ptr<IDXCoreAdapter> spAdapter;
            hr = spAdapterList->GetItem(adapterIndex, spAdapter.put());
            THROW_IF_FAILED(hr);

            CHAR driverDescription[128];

            spAdapter->QueryProperty(DXCoreProperty::DriverDescription, sizeof(driverDescription), driverDescription);
            printf("Use adapter : %s\n", driverDescription);

            pAdapter = spAdapter.get();

            com_ptr<IDXGIAdapter> spDxgiAdapter;

            if (spAdapter->IsDXAttributeSupported(DXCORE_ADAPTER_ATTRIBUTE_D3D12_GRFX))
            {
                d3dFeatureLevel = D3D_FEATURE_LEVEL::D3D_FEATURE_LEVEL_11_0;
                commandQueueType = D3D12_COMMAND_LIST_TYPE_DIRECT;

                com_ptr<IDXGIFactory4> dxgiFactory4;

                try
                {
                    hr = CreateDXGIFactory2SEH(dxgiFactory4.put_void());
                }
                catch (...)
                {
                    hr = E_FAIL;
                }

                if (hr == S_OK)
                {
                    LUID adapterLuid;
                    spAdapter->GetLUID(&adapterLuid);

                    hr = dxgiFactory4->EnumAdapterByLuid(adapterLuid, __uuidof(IDXGIAdapter), spDxgiAdapter.put_void());
                    if (hr == S_OK)
                    {
                        pAdapter = spDxgiAdapter.get();
                    }
                }
            }

            com_ptr<ID3D12Device> d3d12Device;
            hr = D3D12CreateDevice(pAdapter, d3dFeatureLevel, __uuidof(ID3D12Device), d3d12Device.put_void());
            THROW_IF_FAILED(hr);

            com_ptr<ID3D12CommandQueue> d3d12CommandQueue;
            D3D12_COMMAND_QUEUE_DESC commandQueueDesc = {};
            commandQueueDesc.Type = commandQueueType;

            hr = d3d12Device->CreateCommandQueue(&commandQueueDesc, __uuidof(ID3D12CommandQueue), d3d12CommandQueue.put_void());
            THROW_IF_FAILED(hr);

            auto factory = get_activation_factory<LearningModelDevice, ILearningModelDeviceFactoryNative>();

            hr = factory->CreateFromD3D12CommandQueue(d3d12CommandQueue.get(), spUnkLearningModelDevice.put());
            THROW_IF_FAILED(hr);

            session = LearningModelSession(model, spUnkLearningModelDevice.as<LearningModelDevice>());
        }
        else
        {
            LearningModelDevice learningModelDevice(TypeHelper::GetWinmlDeviceKind(deviceType));
            output.PrintLearningModelDevice(deviceType, learningModelDevice);
            if (args.IsPerformanceCapture())
            {
                WINML_PROFILING_START(profiler, WINML_MODEL_TEST_PERF::CREATE_SESSION);
            }
            session = LearningModelSession(model, learningModelDevice);
            if (args.IsPerformanceCapture())
            {
                WINML_PROFILING_STOP(profiler, WINML_MODEL_TEST_PERF::CREATE_SESSION);
            }
        }
    }
    catch (hresult_error hr)
    {
        std::cout << "Creating session [FAILED]" << std::endl;
        std::wcout << hr.message().c_str() << std::endl;
        return hr.code();
    }

    if (args.IsDebugOutputEnabled())
    {
        // Enables trace log output. 
        session.EvaluationProperties().Insert(L"EnableDebugOutput", nullptr);
    }

    LearningModelBinding context(session);

    bool useInputData = false;

    bool isGarbageData = args.IsGarbageInput();
    std::string completionString = "\n";

    // Run the binding + evaluate multiple times and average the results
    for (uint32_t i = 0; i < args.NumIterations(); i++)
    {
        bool captureIterationPerf = args.IsPerformanceCapture() || args.IsPerIterationCapture();

        std::vector<ILearningModelFeatureValue> inputFeatures;
        try
        {
            inputFeatures = GenerateInputFeatures(model, args, inputBindingType, inputDataType, winrtDevice, i);
        }
        catch (hresult_error hr)
        {
            std::wcout << "\nGenerating Input Features [FAILED]" << std::endl;
            std::wcout << hr.message().c_str() << std::endl;
            return hr.code();
        }

        HRESULT bindInputResult = BindInputFeatures(model, context, inputFeatures, args, output, captureIterationPerf, i, profiler);

        if (FAILED(bindInputResult))
        {
            output.PrintBindingInfo(i + 1, deviceType, inputBindingType, inputDataType, deviceCreationLocation, "[FAILED]");
            return bindInputResult;
        }
        else if (!args.TerseOutput() || i == 0)
        {
            output.PrintBindingInfo(i + 1, deviceType, inputBindingType, inputDataType, deviceCreationLocation, "[SUCCESS]");
        }

        LearningModelEvaluationResult result = nullptr;
        HRESULT evalResult = EvaluateModel(result, model, context, session, args, output, captureIterationPerf, i, profiler);

        if (FAILED(evalResult))
        {
            output.PrintEvaluatingInfo(i + 1, deviceType, inputBindingType, inputDataType, deviceCreationLocation, "[FAILED]");
            return evalResult;
        }
        else if (!args.TerseOutput() || i == 0)
        {
            output.PrintEvaluatingInfo(i + 1, deviceType, inputBindingType, inputDataType, deviceCreationLocation, "[SUCCESS]");

            // Only print eval results on the first iteration, iff it's not garbage data
            if (!isGarbageData)
            {
                BindingUtilities::PrintEvaluationResults(model, args, result.Outputs());
            }

            if (args.TerseOutput() && args.NumIterations() > 1)
            {
                printf("Binding and Evaluating %d more time%s...", args.NumIterations()-1, (args.NumIterations() == 2 ? "" : "s"));
                completionString = "[SUCCESS]\n";
            }
        }
    }
    printf("%s", completionString.c_str());

    if (args.IsSaveTensor() || args.IsPerIterationCapture())
    {
        output.WritePerIterationPerformance(args, args.ModelPath(), args.ImagePath());
    }

    // Clean up session resources
    session.Close();
    session.~LearningModelSession();

    return S_OK;
}

HRESULT EvaluateModels(
    const std::vector<std::wstring>& modelPaths,
    const std::vector<DeviceType>& deviceTypes,
    const std::vector<InputBindingType>& inputBindingTypes,
    const std::vector<InputDataType>& inputDataTypes,
    const std::vector<DeviceCreationLocation> deviceCreationLocations,
    const CommandLineArgs& args,
    OutputHelper& output,
    Profiler<WINML_MODEL_TEST_PERF>& profiler
)
{
    output.PrintHardwareInfo();

    for (const auto& path : modelPaths)
    {
        LearningModel model = nullptr;

        try
        {
            model = LoadModel(path, args.IsPerformanceCapture() || args.IsPerIterationCapture(), output, args, 0, profiler);
        }
        catch (hresult_error hr)
        {
            std::cout << hr.message().c_str() << std::endl;
            return hr.code();
        }

        auto firstFeature = model.InputFeatures().First().Current();
        auto tensorDescriptor = firstFeature.try_as<TensorFeatureDescriptor>();

        // Map and Sequence bindings are not supported yet
        if (!tensorDescriptor)
        {
            std::wcout << L"Model: " + path + L" has an input type that isn't supported by WinMLRunner yet." << std::endl;
            continue;
        }

        for (const auto &deviceType : deviceTypes)
        {
            for (const auto &inputBindingType : inputBindingTypes)
            {
                for (const auto &inputDataType : inputDataTypes)
                {
                    for (const auto &deviceCreationLocation : deviceCreationLocations)
                    {
                        if (args.IsPerformanceCapture() || args.IsPerIterationCapture())
                        {
                            // Resets all values from profiler for bind and evaluate.
                            profiler.Reset(WINML_MODEL_TEST_PERF::BIND_VALUE, WINML_MODEL_TEST_PERF::COUNT);
                        }

                        if (inputDataType != InputDataType::Tensor)
                        {
                            // Currently GPU binding only work with 4D tensors and RGBA/BGRA images
                            if (tensorDescriptor.Shape().Size() != 4 || tensorDescriptor.Shape().GetAt(1) != 3)
                            {
                                continue;
                            }
                        }

                        HRESULT evalHResult = EvaluateModel(model, args, output, deviceType, inputBindingType, inputDataType, deviceCreationLocation, profiler);

                        if (FAILED(evalHResult))
                        {
                            return evalHResult;
                        }

                        if (args.IsPerformanceCapture())
                        {
                            output.PrintResults(profiler, args.NumIterations(), deviceType, inputBindingType, inputDataType, deviceCreationLocation, args.IsPerformanceConsoleOutputVerbose());
                            if (args.IsOutputPerf())
                            {
                                std::string deviceTypeStringified = TypeHelper::Stringify(deviceType);
                                std::string inputDataTypeStringified = TypeHelper::Stringify(inputDataType);
                                std::string inputBindingTypeStringified = TypeHelper::Stringify(inputBindingType);
                                std::string deviceCreationLocationStringified = TypeHelper::Stringify(deviceCreationLocation);
                                output.WritePerformanceDataToCSV(
                                    profiler,
                                    args.NumIterations(),
                                    path,
                                    deviceTypeStringified,
                                    inputDataTypeStringified,
                                    inputBindingTypeStringified,
                                    deviceCreationLocationStringified
                                );
                            }
                        }
                    }
                }
            }
        }

        model.Close();
    }

    return S_OK;
}

std::vector<InputDataType> FetchInputDataTypes(const CommandLineArgs& args)
{
    std::vector<InputDataType> inputDataTypes;

    if (args.UseTensor())
    {
        inputDataTypes.push_back(InputDataType::Tensor);
    }

    if (args.UseRGB())
    {
        inputDataTypes.push_back(InputDataType::ImageRGB);
    }

    if (args.UseBGR())
    {
        inputDataTypes.push_back(InputDataType::ImageBGR);
    }

    return inputDataTypes;
}

std::vector<DeviceType> FetchDeviceTypes(const CommandLineArgs& args)
{
    std::vector<DeviceType> deviceTypes;

    if (args.UseCPU())
    {
        deviceTypes.push_back(DeviceType::CPU);
    }

    if (args.UseGPU())
    {
        deviceTypes.push_back(DeviceType::DefaultGPU);
    }

    if (args.IsUsingGPUHighPerformance())
    {
        deviceTypes.push_back(DeviceType::HighPerfGPU);
    }

    if (args.IsUsingGPUMinPower())
    {
        deviceTypes.push_back(DeviceType::MinPowerGPU);
    }

    return deviceTypes;
}

std::vector<InputBindingType> FetchInputBindingTypes(const CommandLineArgs& args)
{
    std::vector<InputBindingType> inputBindingTypes;

    if (args.UseCPUBoundInput())
    {
        inputBindingTypes.push_back(InputBindingType::CPU);
    }

    if (args.IsUsingGPUBoundInput())
    {
        inputBindingTypes.push_back(InputBindingType::GPU);
    }

    return inputBindingTypes;
}

std::vector<DeviceCreationLocation> FetchDeviceCreationLocations(const CommandLineArgs& args)
{
    std::vector<DeviceCreationLocation> deviceCreationLocations;

    if (args.CreateDeviceInWinML())
    {
        deviceCreationLocations.push_back(DeviceCreationLocation::WinML);
    }

    if (args.IsCreateDeviceOnClient())
    {
        deviceCreationLocations.push_back(DeviceCreationLocation::ClientCode);
    }

    return deviceCreationLocations;
}

HRESULT CreateDXGIFactory2SEH(void **pIDXGIFactory)
{
    HRESULT hr;

    __try
    {
        hr = CreateDXGIFactory2(0, __uuidof(IDXGIFactory4), pIDXGIFactory);
    }
    __except (GetExceptionCode() == VcppException(ERROR_SEVERITY_ERROR, ERROR_MOD_NOT_FOUND) ?
        EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
    {
        hr = HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
    }

    return hr;
}

int run(CommandLineArgs& args, Profiler<WINML_MODEL_TEST_PERF>& profiler)
{
    // Initialize COM in a multi-threaded environment.
    winrt::init_apartment();
    OutputHelper output(args.NumIterations());

    // Profiler is a wrapper class that captures and stores timing and memory usage data on the
    // CPU and GPU.
    profiler.Enable();

    if (!args.OutputPath().empty())
    {
        output.SetCSVFileName(args.OutputPath());
    }
    else
    {
        output.SetDefaultCSVFileName();
    }

    if (args.IsSaveTensor() || args.IsPerIterationCapture())
    {
        output.SetDefaultPerIterationFolder();
        output.SetDefaultCSVFileNamePerIteration();
    }

    D3D12EnableExperimentalFeatures(1, &D3D12ComputeOnlyDevices, NULL, 0);

    if (!args.ModelPath().empty() || !args.FolderPath().empty())
    {
        std::vector<DeviceType> deviceTypes = FetchDeviceTypes(args);
        std::vector<InputBindingType> inputBindingTypes = FetchInputBindingTypes(args);
        std::vector<InputDataType> inputDataTypes = FetchInputDataTypes(args);
        std::vector<DeviceCreationLocation> deviceCreationLocations = FetchDeviceCreationLocations(args);
        std::vector<std::wstring> modelPaths = args.ModelPath().empty() ? GetModelsInDirectory(args, &output) : std::vector<std::wstring>(1, args.ModelPath());

        if (args.IsConcurrentLoad()) {
            ConcurrentLoadModel(modelPaths[0], args.NumThreads(), args.ThreadInterval(), true);
            return 0;
        }
        return EvaluateModels(modelPaths, deviceTypes, inputBindingTypes, inputDataTypes, deviceCreationLocations, args, output, profiler);
    }

    return 0;
}
