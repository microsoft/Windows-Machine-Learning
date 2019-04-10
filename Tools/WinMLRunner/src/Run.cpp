#include "Common.h"
#include "OutputHelper.h"
#include "ModelBinding.h"
#include "BindingUtilities.h"
#include <filesystem>
#include <d3d11.h>
#include <Windows.Graphics.DirectX.Direct3D11.interop.h>
#include "Run.h"
#include "Scenarios.h"

using namespace winrt::Windows::Graphics::DirectX::Direct3D11;

std::vector<ILearningModelFeatureValue> GenerateInputFeatures(const LearningModel& model, const CommandLineArgs& args,
                                                              InputBindingType inputBindingType,
                                                              InputDataType inputDataType,
                                                              const IDirect3DDevice winrtDevice, uint32_t iterationNum)
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
            auto imageFeature = BindingUtilities::CreateBindableImage(description, args.ImagePath(), inputBindingType,
                                                                      inputDataType, winrtDevice, args, iterationNum);
            inputFeatures.push_back(imageFeature);
        }
    }

    return inputFeatures;
}

HRESULT BindInputFeatures(const LearningModel& model, const LearningModelBinding& context,
                          const std::vector<ILearningModelFeatureValue>& inputFeatures, const CommandLineArgs& args,
                          OutputHelper& output, bool capturePerf, uint32_t iterationNum,
                          Profiler<WINML_MODEL_TEST_PERF>& profiler)
{
    assert(model.InputFeatures().Size() == inputFeatures.size());

    try
    {
        context.Clear();

        if (capturePerf)
        {
            WINML_PROFILING_START(profiler, iterationNum == 0 ? WINML_MODEL_TEST_PERF::BIND_VALUE_FIRST_RUN
                                                              : WINML_MODEL_TEST_PERF::BIND_VALUE);
        }

        for (uint32_t i = 0; i < model.InputFeatures().Size(); i++)
        {
            auto&& description = model.InputFeatures().GetAt(i);
            context.Bind(description.Name(), inputFeatures[i]);
        }

        if (capturePerf)
        {
            WINML_PROFILING_STOP(profiler, iterationNum == 0 ? WINML_MODEL_TEST_PERF::BIND_VALUE_FIRST_RUN
                                                             : WINML_MODEL_TEST_PERF::BIND_VALUE);
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

LearningModel LoadModel(const std::wstring& path, bool capturePerf, OutputHelper& output, const CommandLineArgs& args,
                        uint32_t iterationNum, Profiler<WINML_MODEL_TEST_PERF>& profiler)
{
    LearningModel model = nullptr;
    output.PrintLoadingInfo(path);
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
    output.PrintModelInfo(path, model);

    return model;
}

HRESULT CreateSession(LearningModelSession& session, IDirect3DDevice &winrtDevice, LearningModel& model, CommandLineArgs& args, OutputHelper& output,
                      DeviceType deviceType, DeviceCreationLocation deviceCreationLocation,
                      Profiler<WINML_MODEL_TEST_PERF>& profiler)
{
    typedef std::pair<HRESULT, LearningModelSession> pair;
    if (model == nullptr)
    {
        return hresult_invalid_argument().code();
    }

    try
    {
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
                    hr = factory->EnumAdapterByGpuPreference(0, DXGI_GPU_PREFERENCE_UNSPECIFIED, __uuidof(IDXGIAdapter),
                                                             adapter.put_void());
                    break;
                case DeviceType::MinPowerGPU:
                    hr = factory->EnumAdapterByGpuPreference(0, DXGI_GPU_PREFERENCE_MINIMUM_POWER,
                                                             __uuidof(IDXGIAdapter), adapter.put_void());
                    break;
                case DeviceType::HighPerfGPU:
                    hr = factory->EnumAdapterByGpuPreference(0, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
                                                             __uuidof(IDXGIAdapter), adapter.put_void());
                    break;
                default:
                    throw hresult(E_INVALIDARG);
            }
            THROW_IF_FAILED(hr);

            // Creating the device on the client and using it to create the video frame and initialize the session makes
            // sure that everything is on the same device. This usually avoids an expensive cross-device and
            // cross-videoframe copy via the VideoFrame pipeline.
            com_ptr<ID3D11Device> d3d11Device;
            hr = D3D11CreateDevice(adapter.get(), D3D_DRIVER_TYPE_UNKNOWN, nullptr, D3D11_CREATE_DEVICE_BGRA_SUPPORT,
                                   nullptr, 0, D3D11_SDK_VERSION, d3d11Device.put(), nullptr, nullptr);
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

    if (args.IsEvaluationDebugOutputEnabled())
    {
        // Enables trace log output.
        session.EvaluationProperties().Insert(L"EnableDebugOutput", nullptr);
    }

    return S_OK;
}

HRESULT BindInputs(LearningModelBinding &context, const LearningModel& model, const LearningModelSession& session, OutputHelper& output,
                   DeviceType deviceType, const CommandLineArgs& args, InputBindingType inputBindingType, InputDataType inputDataType,
                   const IDirect3DDevice winrtDevice, DeviceCreationLocation deviceCreationLocation, uint32_t iteration,
                   Profiler<WINML_MODEL_TEST_PERF>& profiler)
{
    bool useInputData = false;
    bool isGarbageData = args.IsGarbageInput();
    std::string completionString = "\n";

    // Run the binding + evaluate multiple times and average the results
    bool captureIterationPerf = args.IsPerformanceCapture() || args.IsPerIterationCapture();

    std::vector<ILearningModelFeatureValue> inputFeatures;
    try
    {
        inputFeatures = GenerateInputFeatures(model, args, inputBindingType, inputDataType, winrtDevice, iteration);
    }
    catch (hresult_error hr)
    {
        std::wcout << "\nGenerating Input Features [FAILED]" << std::endl;
        std::wcout << hr.message().c_str() << std::endl;
        return hr.code();
    }


    HRESULT bindInputResult =
        BindInputFeatures(model, context, inputFeatures, args, output, captureIterationPerf, iteration, profiler);

    if (FAILED(bindInputResult))
    {
        output.PrintBindingInfo(iteration + 1, deviceType, inputBindingType, inputDataType, deviceCreationLocation,
                                "[FAILED]");
        return bindInputResult;
    }
    else if (!args.TerseOutput() || iteration == 0)
    {
        output.PrintBindingInfo(iteration + 1, deviceType, inputBindingType, inputDataType, deviceCreationLocation,
                                "[SUCCESS]");
    }
    return S_OK;
}

std::vector<std::wstring> GetModelsInDirectory(CommandLineArgs& args, OutputHelper* output)
{
    std::vector<std::wstring> modelPaths;

    std::wstring folderPath = args.FolderPath();
    for (auto& it : std::filesystem::directory_iterator(args.FolderPath()))
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

HRESULT EvaluateModel(LearningModelEvaluationResult& result, const LearningModel& model,
                      const LearningModelBinding& context, LearningModelSession& session, const CommandLineArgs& args,
                      OutputHelper& output, bool capturePerf, uint32_t iterationNum,
                      Profiler<WINML_MODEL_TEST_PERF>& profiler)
{
    try
    {
        if (capturePerf)
        {
            WINML_PROFILING_START(profiler, iterationNum == 0 ? WINML_MODEL_TEST_PERF::EVAL_MODEL_FIRST_RUN
                                                              : WINML_MODEL_TEST_PERF::EVAL_MODEL);
        }

        result = session.Evaluate(context, L"");

        if (capturePerf)
        {
            WINML_PROFILING_STOP(profiler, iterationNum == 0 ? WINML_MODEL_TEST_PERF::EVAL_MODEL_FIRST_RUN
                                                             : WINML_MODEL_TEST_PERF::EVAL_MODEL);
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

    // Only print eval results on the first iteration, iff it's not garbage data
    if (!args.IsGarbageInput() || args.IsSaveTensor())
    {
        BindingUtilities::PrintOrSaveEvaluationResults(model, args, result.Outputs(), output, iterationNum);
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

int run(CommandLineArgs& args, Profiler<WINML_MODEL_TEST_PERF>& profiler)
{
    // Initialize COM in a multi-threaded environment.
    winrt::init_apartment();
    OutputHelper output(args.NumIterations());

#if defined(_AMD64_)
    // PIX markers only work on AMD64
    // Check if PIX tool is attached to WinMLRunner
    // Try to acquire IDXGraphicsAnalysis - this only succeeds if PIX is attached
    if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(output.GetGraphicsAnalysis().put()))))
    {
        std::cout << "Detected PIX tool is attached to WinMLRunner" << std::endl;
    }
#endif

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
        output.SetDefaultPerIterationFolder(args.TensorOutputPath());
        output.SetDefaultCSVFileNamePerIteration();
    }

    if (!args.ModelPath().empty() || !args.FolderPath().empty())
    {
        std::vector<DeviceType> deviceTypes = FetchDeviceTypes(args);
        std::vector<InputBindingType> inputBindingTypes = FetchInputBindingTypes(args);
        std::vector<InputDataType> inputDataTypes = FetchInputDataTypes(args);
        std::vector<DeviceCreationLocation> deviceCreationLocations = FetchDeviceCreationLocations(args);
        std::vector<std::wstring> modelPaths = args.ModelPath().empty()
                                                   ? GetModelsInDirectory(args, &output)
                                                   : std::vector<std::wstring>(1, args.ModelPath());

        if (args.IsConcurrentLoad())
        {
            ConcurrentLoadModel(modelPaths, args.NumThreads(), args.ThreadInterval(), true);
            return 0;
        }

        for (const auto& path: modelPaths)
        {
            LearningModel model = nullptr;
            try
            {
                model = LoadModel(path, args.IsPerformanceCapture() || args.IsPerIterationCapture(), output, args, 0, profiler);
            }
            catch (hresult_error hr)
            {
                std::wcout << "Load Model: " << path << " [FAILED]" << std::endl;
                std::wcout << hr.message().c_str() << std::endl;
                return hr.code();
            }

            auto firstFeature = model.InputFeatures().First().Current();
            auto tensorDescriptor = firstFeature.try_as<TensorFeatureDescriptor>();

            for (auto deviceType : deviceTypes)
            {
                for (auto deviceCreationLocation : deviceCreationLocations)
                {
#if defined(_AMD64_)
                    // PIX markers only work on AMD64
                    if (output.GetGraphicsAnalysis().get())
                    {
                        // If PIX tool is attached to WinMLRunner then begin capture. First capture will include
                        // session creation, first iteration bind and first iteration evaluate.
                        output.GetGraphicsAnalysis()->BeginCapture();
                    }
#endif
                    LearningModelSession session = nullptr;
                    IDirect3DDevice winrtDevice = nullptr;
                    HRESULT hr = CreateSession(session, winrtDevice, model, args, output, deviceType,
                                               deviceCreationLocation, profiler);
                    for (auto inputDataType : inputDataTypes)
                    {
                       for (auto inputBindingType : inputBindingTypes)
                       {
                            for (uint32_t i = 0; i < args.NumIterations(); i++)
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

#if defined(_AMD64_)
                                // PIX markers only work on AMD64
                                // If PIX tool was attached then capture already began for the first iteration before session creation.
                                // This is to begin PIX capture for each iteration after the first iteration.
                                if (i > 0 && output.GetGraphicsAnalysis())
                                {
                                    output.GetGraphicsAnalysis()->BeginCapture();
                                }
#endif
                                LearningModelBinding context(session);
                                hr = BindInputs(context, model, session, output, deviceType, args, inputBindingType,
                                                inputDataType, winrtDevice, deviceCreationLocation, i, profiler);

                                LearningModelEvaluationResult result = nullptr;
                                bool capture_perf = args.IsPerformanceCapture() || args.IsPerIterationCapture();
                                hr = EvaluateModel(result, model, context, session, args, output,
                                                   capture_perf, i, profiler);

                                if (FAILED(hr))
                                {
                                    output.PrintEvaluatingInfo(i + 1, deviceType, inputBindingType, inputDataType,
                                                               deviceCreationLocation, "[FAILED]");
                                    return hr;
                                }
                                else if (!args.TerseOutput() || i == 0)
                                {
                                    output.PrintEvaluatingInfo(i + 1, deviceType, inputBindingType, inputDataType,
                                                               deviceCreationLocation, "[SUCCESS]");
                                    if (args.TerseOutput() && args.NumIterations() > 1)
                                    {
                                        printf("Binding and Evaluating %d more time%s...", args.NumIterations() - 1,
                                               (args.NumIterations() == 2 ? "" : "s"));
                                    }
                                }
#if defined(_AMD64_)
                                // PIX markers only work on AMD64
                                if (output.GetGraphicsAnalysis())
                                {
                                    // If PIX tool is attached, then end the capture
                                    output.GetGraphicsAnalysis()->EndCapture();
                                }
#endif
                                if (args.IsPerformanceCapture())
                                {
                                    output.PrintResults(profiler, args.NumIterations(), deviceType, inputBindingType, inputDataType,
                                                        deviceCreationLocation, args.IsPerformanceConsoleOutputVerbose());
                                    if (args.IsOutputPerf())
                                    {
                                        std::string deviceTypeStringified = TypeHelper::Stringify(deviceType);
                                        std::string inputDataTypeStringified = TypeHelper::Stringify(inputDataType);
                                        std::string inputBindingTypeStringified = TypeHelper::Stringify(inputBindingType);
                                        std::string deviceCreationLocationStringified = TypeHelper::Stringify(deviceCreationLocation);
                                        output.WritePerformanceDataToCSV(
                                            profiler, args.NumIterations(), path, deviceTypeStringified, inputDataTypeStringified,
                                            inputBindingTypeStringified, deviceCreationLocationStringified);
                                    }
                                }
                            }
                       }
                    }
                }
            }
        }
    }
    return 0;
}
