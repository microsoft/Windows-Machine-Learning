#include "Run.h"
#include "Common.h"
#include "OutputHelper.h"
#include "BindingUtilities.h"
#include "debugoperatorprovider.h"
#include <filesystem>
#include <d3d11.h>
#include <Windows.Graphics.DirectX.Direct3D11.interop.h>
#include "Scenarios.h"
#include <winrt/Windows.Foundation.Metadata.h>
using namespace winrt::Windows::Graphics::DirectX::Direct3D11;
using namespace winrt::Windows::Foundation::Metadata;
std::vector<ILearningModelFeatureValue> GenerateInputFeatures(const LearningModel& model, const CommandLineArgs& args,
                                                              InputBindingType inputBindingType,
                                                              InputDataType inputDataType,
                                                              const LearningModelDeviceWithMetadata& device, uint32_t iterationNum,
                                                              const std::wstring& imagePath)
{
    std::vector<ILearningModelFeatureValue> inputFeatures;
    if (!imagePath.empty() && (!args.TerseOutput() || args.TerseOutput() && iterationNum == 0))
    {
        std::wcout << L"Generating input feature(s) with image: " << imagePath << std::endl;
    }
    for (uint32_t inputNum = 0; inputNum < model.InputFeatures().Size(); inputNum++)
    {
        auto&& description = model.InputFeatures().GetAt(inputNum);

        if (inputDataType == InputDataType::Tensor)
        {
            // If CSV data is provided, then every input will contain the same CSV data
            auto tensorFeature = BindingUtilities::CreateBindableTensor(description, imagePath, inputBindingType, inputDataType,
                                                                        args, iterationNum);
            inputFeatures.push_back(tensorFeature);
        }
        else
        {
            auto imageFeature = BindingUtilities::CreateBindableImage(
                description, imagePath, inputBindingType, inputDataType, device.LearningModelDevice.Direct3D11Device(),
                args, iterationNum);
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

HRESULT LoadModel(LearningModel& model, const std::wstring& path, bool capturePerf, OutputHelper& output,
                  const CommandLineArgs& args, uint32_t iterationNum, Profiler<WINML_MODEL_TEST_PERF>& profiler)
{
    try
    {
        output.PrintLoadingInfo(path);
        for (uint32_t loadIteration = 0; loadIteration < args.NumLoadIterations(); loadIteration++)
        {
            if (capturePerf)
            {
                WINML_PROFILING_START(profiler, WINML_MODEL_TEST_PERF::LOAD_MODEL);
            }
            auto customOperatorProvider = winrt::make<DebugOperatorProvider>();
            auto provider = customOperatorProvider.as<ILearningModelOperatorProvider>();
            model = LearningModel::LoadFromFilePath(path, provider);

            if (capturePerf)
            {
                WINML_PROFILING_STOP(profiler, WINML_MODEL_TEST_PERF::LOAD_MODEL);
                if (args.IsPerIterationCapture())
                {
                    output.SaveLoadTimes(profiler, iterationNum);
                }
            }
        }
        output.PrintModelInfo(path, model);
    }
    catch (hresult_error hr)
    {
        std::wcout << "Load Model: " << path << " [FAILED]" << std::endl;
        std::wcout << hr.message().c_str() << std::endl;
        throw;
    }
    return S_OK;
}

void PopulateSessionOptions(LearningModelSessionOptions& sessionOptions)
{
    // Batch Size Override as 1
    try
    {
        sessionOptions.BatchSizeOverride(1);
    }
    catch (...)
    {
        printf("Batch size override couldn't be set.\n");
        throw;
    }
}

void CreateSessionConsideringSupportForSessionOptions(LearningModelSession& session,
                                                      LearningModel& model,
                                                      Profiler<WINML_MODEL_TEST_PERF>& profiler,
                                                      CommandLineArgs& args,
                                                      const LearningModelDeviceWithMetadata& learningModelDevice)
{
    auto statics = get_activation_factory<ApiInformation, IApiInformationStatics>();
    bool isSessionOptionsTypePresent = isSessionOptionsTypePresent =
        statics.IsTypePresent(L"Windows.AI.MachineLearning.LearningModelSessionOptions");
    if (isSessionOptionsTypePresent)
    {
        LearningModelSessionOptions sessionOptions;
        PopulateSessionOptions(sessionOptions);
        if (args.IsPerformanceCapture())
        {
            WINML_PROFILING_START(profiler, WINML_MODEL_TEST_PERF::CREATE_SESSION);
        }
        session = LearningModelSession(model, learningModelDevice.LearningModelDevice, sessionOptions);
        if (args.IsPerformanceCapture())
        {
            WINML_PROFILING_STOP(profiler, WINML_MODEL_TEST_PERF::CREATE_SESSION);
        }
    }
    else
    {
        if (args.IsPerformanceCapture())
        {
            WINML_PROFILING_START(profiler, WINML_MODEL_TEST_PERF::CREATE_SESSION);
        }
        session = LearningModelSession(model, learningModelDevice.LearningModelDevice);
        if (args.IsPerformanceCapture())
        {
            WINML_PROFILING_STOP(profiler, WINML_MODEL_TEST_PERF::CREATE_SESSION);
        }
    }
}

HRESULT CreateSession(LearningModelSession& session, LearningModel& model, const LearningModelDeviceWithMetadata& learningModelDevice,
                      CommandLineArgs& args, OutputHelper& output, Profiler<WINML_MODEL_TEST_PERF>& profiler)
{
    if (model == nullptr)
    {
        return hresult_invalid_argument().code();
    }
    try
    {
        output.PrintLearningModelDevice(learningModelDevice);
        CreateSessionConsideringSupportForSessionOptions(session, model, profiler, args, learningModelDevice);
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

HRESULT BindInputs(LearningModelBinding& context, const LearningModelSession& session,
                   OutputHelper& output, const LearningModelDeviceWithMetadata& device, const CommandLineArgs& args,
                   InputBindingType inputBindingType, InputDataType inputDataType, uint32_t iteration,
                   Profiler<WINML_MODEL_TEST_PERF>& profiler, const std::wstring& imagePath)
{
    if (device.DeviceType == DeviceType::CPU && inputDataType == InputDataType::Tensor &&
        inputBindingType == InputBindingType::GPU)
    {
        std::cout << "Cannot create D3D12 device on client if CPU device type is selected." << std::endl;
        return E_INVALIDARG;
    }
    bool useInputData = false;
    bool isGarbageData = args.IsGarbageInput();
    std::string completionString = "\n";

    // Run the binding + evaluate multiple times and average the results
    bool captureIterationPerf = args.IsPerformanceCapture() || args.IsPerIterationCapture();

    std::vector<ILearningModelFeatureValue> inputFeatures;
    try
    {
        inputFeatures = GenerateInputFeatures(session.Model(), args, inputBindingType, inputDataType, device, iteration, imagePath);
    }
    catch (hresult_error hr)
    {
        std::wcout << "\nGenerating Input Features [FAILED]" << std::endl;
        std::wcout << hr.message().c_str() << std::endl;
        return hr.code();
    }

    HRESULT bindInputResult =
        BindInputFeatures(session.Model(), context, inputFeatures, args, output, captureIterationPerf, iteration, profiler);

    if (FAILED(bindInputResult))
    {
        output.PrintBindingInfo(iteration + 1, device.DeviceType, inputBindingType, inputDataType, device.DeviceCreationLocation,
                                "[FAILED]");
        return bindInputResult;
    }
    else if (!args.TerseOutput() || iteration == 0)
    {
        output.PrintBindingInfo(iteration + 1, device.DeviceType, inputBindingType, inputDataType,
                                device.DeviceCreationLocation,
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

HRESULT CheckIfModelAndConfigurationsAreSupported(LearningModel& model, const std::wstring& modelPath,
                                                  const DeviceType deviceType,
                                                  const std::vector<InputDataType>& inputDataTypes)
{
    // Does user want image as input binding
    bool hasInputBindingImage =
        std::any_of(inputDataTypes.begin(), inputDataTypes.end(), [](const InputDataType inputDataType) {
            return inputDataType == InputDataType::ImageBGR || inputDataType == InputDataType::ImageRGB;
        });

    for (auto inputFeature : model.InputFeatures())
    {
        if (inputFeature.Kind() != LearningModelFeatureKind::Tensor &&
            inputFeature.Kind() != LearningModelFeatureKind::Image)
        {
            std::wcout << L"Model: " + modelPath + L" has an input type that isn't supported by WinMLRunner."
                       << std::endl;
            return E_NOTIMPL;
        }
        else if (inputFeature.Kind() == LearningModelFeatureKind::Tensor)
        {
            auto tensorFeatureDescriptor = inputFeature.try_as<TensorFeatureDescriptor>();
            if (tensorFeatureDescriptor.Shape().Size() > 4 && deviceType != DeviceType::CPU)
            {
                std::cout << "Input feature " << to_string(inputFeature.Name())
                          << " shape is too large. GPU path only accepts tensor dimensions <= 4 : "
                          << tensorFeatureDescriptor.Shape().Size() << std::endl;
                return E_INVALIDARG;
            }

            // If image as input binding, then the model's tensor inputs should have channel 3 or 1
            if (hasInputBindingImage &&
                (tensorFeatureDescriptor.Shape().Size() != 4 ||
                 (tensorFeatureDescriptor.Shape().GetAt(1) != 1 && tensorFeatureDescriptor.Shape().GetAt(1) != 3)))
            {

                std::cout << "Attempting to bind image but input feature " << to_string(inputFeature.Name())
                          << " shape is invalid. Shape should be 4 dimensions (NCHW) with C = 3." << std::endl;
                return E_INVALIDARG;
            }
        }
    }
    return S_OK;
}

HRESULT EvaluateModel(LearningModelEvaluationResult& result,
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
    return S_OK;
}

#if defined(_AMD64_)
void StartPIXCapture(OutputHelper& output)
{
    __try
    {
        // PIX markers only work on AMD64
        if (output.GetGraphicsAnalysis().get())
        {
            // If PIX tool is attached to WinMLRunner then begin capture. First capture will include
            // session creation, first iteration bind and first iteration evaluate.
            output.GetGraphicsAnalysis()->BeginCapture();
        }
    }
    __except (GetExceptionCode() == VcppException(ERROR_SEVERITY_ERROR, ERROR_MOD_NOT_FOUND)
                  ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
    {
        std::cout << "DXGI module not found." << std::endl;
    }
}

void EndPIXCapture(OutputHelper& output)
{
    __try
    {
        // PIX markers only work on AMD64
        if (output.GetGraphicsAnalysis().get())
        {
            // If PIX tool is attached to WinMLRunner then end capture.
            output.GetGraphicsAnalysis()->EndCapture();
        }
    }
    __except (GetExceptionCode() == VcppException(ERROR_SEVERITY_ERROR, ERROR_MOD_NOT_FOUND)
                  ? EXCEPTION_EXECUTE_HANDLER
                  : EXCEPTION_CONTINUE_SEARCH)
    {
        std::cout << "DXGI module not found." << std::endl;
    }
}

void PrintIfPIXToolAttached(OutputHelper& output)
{
    __try
    {
        // PIX markers only work on AMD64
        // Check if PIX tool is attached to WinMLRunner
        // Try to acquire IDXGraphicsAnalysis - this only succeeds if PIX is attached
        if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(output.GetGraphicsAnalysis().put()))))
        {
            std::cout << "Detected PIX tool is attached to WinMLRunner" << std::endl;
        }
    }
    __except (GetExceptionCode() == VcppException(ERROR_SEVERITY_ERROR, ERROR_MOD_NOT_FOUND)
                  ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
    {
        std::cout << "DXGI module not found." << std::endl;
    }
}
#endif

void IterateBindAndEvaluate(const int maxBindAndEvalIterations, int& lastIteration, CommandLineArgs& args, OutputHelper& output,
                            LearningModelSession& session, HRESULT& lastHr,
                            const LearningModelDeviceWithMetadata& device, const InputBindingType inputBindingType,
                            const InputDataType inputDataType,
                            Profiler<WINML_MODEL_TEST_PERF>& profiler, const std::wstring& imagePath)
{
    Timer iterationTimer;
    for (; lastIteration < maxBindAndEvalIterations; lastIteration++)
    {
#if defined(_AMD64_)
        // PIX markers only work on AMD64
        // If PIX tool was attached then capture already began for the first iteration before
        // session creation. This is to begin PIX capture for each iteration after the first
        // iteration.
        if (lastIteration > 0)
        {
            StartPIXCapture(output);
        }
#endif
        if (args.IsTimeLimitIterations())
        {
            if (lastIteration == 1)
            {
                iterationTimer.Start();
            }
            else if (lastIteration >= 1 && iterationTimer.Stop() >= args.IterationTimeLimit())
            {
                std::cout << "Iteration time exceeded limit specified. Exiting.." << std::endl;
                break;
            }
        }
        LearningModelBinding context(session);
        lastHr = BindInputs(context, session, output, device, args, inputBindingType, inputDataType, lastIteration, profiler, imagePath);
        if (FAILED(lastHr))
        {
            break;
        }
        LearningModelEvaluationResult result = nullptr;
        bool capture_perf = args.IsPerformanceCapture() || args.IsPerIterationCapture();
        lastHr = EvaluateModel(result, context, session, args, output, capture_perf, lastIteration, profiler);
        if (FAILED(lastHr))
        {
            output.PrintEvaluatingInfo(lastIteration + 1, device.DeviceType, inputBindingType, inputDataType,
                                       device.DeviceCreationLocation, "[FAILED]");
            break;
        }
        else if (!args.TerseOutput() || lastIteration == 0)
        {
            output.PrintEvaluatingInfo(lastIteration + 1, device.DeviceType, inputBindingType, inputDataType,
                                       device.DeviceCreationLocation, "[SUCCESS]");

            // Only print eval results on the first iteration, iff it's not garbage data
            if (!args.IsGarbageInput() || args.IsSaveTensor())
            {
                BindingUtilities::PrintOrSaveEvaluationResults(session.Model(), args, result.Outputs(), output, lastIteration);
            }

            if (args.TerseOutput() && args.NumIterations() > 1)
            {
                printf("Binding and Evaluating %d more time%s...", args.NumIterations() - 1,
                       (args.NumIterations() == 2 ? "" : "s"));
            }
        }
#if defined(_AMD64_)
        EndPIXCapture(output);
#endif
    }
}

void RunBindAndEvaluateOnce(CommandLineArgs& args, OutputHelper& output, LearningModelSession& session,
                            HRESULT& lastHr, const LearningModelDeviceWithMetadata& device,
                            const InputBindingType inputBindingType, const InputDataType inputDataType,
                            Profiler<WINML_MODEL_TEST_PERF>& profiler, const std::wstring& imagePath)
{
    int lastIteration = 0;
    IterateBindAndEvaluate(1, lastIteration, args, output, session, lastHr, device, inputBindingType, inputDataType,
                           profiler, imagePath);
}

void WritePerfResults(CommandLineArgs& args, OutputHelper& output, LearningModelSession& session,
                      const LearningModelDeviceWithMetadata& device, const InputBindingType inputBindingType,
                      const InputDataType inputDataType, Profiler<WINML_MODEL_TEST_PERF>& profiler,
                      const std::wstring& modelPath, const std::wstring& imagePath,
                      const uint32_t sessionCreationIteration, const int lastIteration)
{
    output.PrintResults(profiler, lastIteration, device.DeviceType, inputBindingType, inputDataType, device.DeviceCreationLocation,
                        args.IsPerformanceConsoleOutputVerbose());
    if (args.IsOutputPerf())
    {
        std::string deviceTypeStringified = TypeHelper::Stringify(device.DeviceType);
        std::string inputDataTypeStringified = TypeHelper::Stringify(inputDataType);
        std::string inputBindingTypeStringified = TypeHelper::Stringify(inputBindingType);
        std::string deviceCreationLocationStringified = TypeHelper::Stringify(device.DeviceCreationLocation);
        output.WritePerformanceDataToCSV(profiler, lastIteration, modelPath, deviceTypeStringified,
                                            inputDataTypeStringified, inputBindingTypeStringified,
                                            deviceCreationLocationStringified, args.GetPerformanceFileMetadata());
    }
    if (args.IsPerIterationCapture())
    {
        output.WritePerIterationPerformance(args, session.Model().Name().c_str(), imagePath);
    }
}

void RunConfiguration(CommandLineArgs& args, OutputHelper& output, LearningModelSession& session, HRESULT& lastHr,
                      const InputBindingType inputBindingType, const InputDataType inputDataType,
                      Profiler<WINML_MODEL_TEST_PERF>& profiler, const std::wstring& modelPath,
                      const std::wstring& imagePath, const uint32_t sessionCreationIteration, const LearningModelDeviceWithMetadata& device)
{
    if (sessionCreationIteration < args.NumSessionCreationIterations() - 1)
    {
        RunBindAndEvaluateOnce(args, output, session, lastHr, device, inputBindingType, inputDataType, profiler, imagePath);
        return;
    }
    else
    {
        int lastIteration = 0;
        IterateBindAndEvaluate(args.NumIterations(), lastIteration, args, output, session, lastHr, device,
                               inputBindingType, inputDataType, profiler, imagePath);
        if (args.IsPerformanceCapture() && SUCCEEDED(lastHr))
        {
            WritePerfResults(args, output, session, device, inputBindingType, inputDataType, profiler, modelPath,
                             imagePath, sessionCreationIteration, lastIteration);
        }
    }
}
int run(CommandLineArgs& args,
        Profiler<WINML_MODEL_TEST_PERF>& profiler,
        const std::vector<LearningModelDeviceWithMetadata>& deviceList) try
{
    // Initialize COM in a multi-threaded environment.
    winrt::init_apartment();
    OutputHelper output(args.NumIterations());

#if defined(_AMD64_)
    PrintIfPIXToolAttached(output);
#endif

    // Profiler is a wrapper class that captures and stores timing and memory usage data on the
    // CPU and GPU.
    profiler.Enable();

    output.SetCSVFileName(args.OutputPath());
    if (args.IsSaveTensor() || args.IsPerIterationCapture())
    {
        output.SetDefaultPerIterationFolder(args.PerIterationDataPath());
        output.SetDefaultCSVFileNamePerIteration();
    }

    if (!args.ModelPath().empty() || !args.FolderPath().empty())
    {
        std::vector<InputBindingType> inputBindingTypes = args.FetchInputBindingTypes();
        std::vector<InputDataType> inputDataTypes = args.FetchInputDataTypes();
        std::vector<std::wstring> modelPaths = args.ModelPath().empty()
                                                   ? GetModelsInDirectory(args, &output)
                                                   : std::vector<std::wstring>(1, args.ModelPath());
        HRESULT lastHr = S_OK;
        if (args.IsConcurrentLoad())
        {
            ConcurrentLoadModel(modelPaths, args.NumThreads(), args.ThreadInterval(), true);
            return 0;
        }
        for (const auto& path : modelPaths)
        {
            LearningModel model = nullptr;

            LoadModel(model, path, args.IsPerformanceCapture() || args.IsPerIterationCapture(), output, args, 0,
                      profiler);
            for (auto& learningModelDevice : deviceList)
            {
                lastHr = CheckIfModelAndConfigurationsAreSupported(model, path, learningModelDevice.DeviceType, inputDataTypes);
                if (FAILED(lastHr))
                {
                    continue;
                }
#if defined(_AMD64_)
                StartPIXCapture(output);
#endif
                LearningModelSession session = nullptr;
                for (auto inputDataType : inputDataTypes)
                {
                    for (auto inputBindingType : inputBindingTypes)
                    {
                        // Clear up session, bind, eval performance metrics after configuration iteration
                        if (args.IsPerformanceCapture() || args.IsPerIterationCapture())
                        {
                            // Resets all values from profiler for bind and evaluate.
                            profiler.Reset(WINML_MODEL_TEST_PERF::BIND_VALUE, WINML_MODEL_TEST_PERF::COUNT);
                        }
                        for (uint32_t sessionCreationIteration = 0;
                            sessionCreationIteration < args.NumSessionCreationIterations();
                            sessionCreationIteration++)
                        {
                            lastHr = CreateSession(session, model, learningModelDevice,args, output, profiler);
                            if (FAILED(lastHr))
                            {
                                continue;
                            }
                            if (args.IsImageInput())
                            {
                                for (const std::wstring& inputImagePath : args.ImagePaths())
                                {
                                    RunConfiguration(args, output, session, lastHr, inputBindingType, inputDataType,
                                                     profiler, path, inputImagePath, sessionCreationIteration,
                                                     learningModelDevice);
                                }
                            }
                            else
                            {
                                RunConfiguration(args, output, session, lastHr, inputBindingType, inputDataType,
                                                 profiler, path, L"", sessionCreationIteration,
                                                 learningModelDevice);
                            }
                            // Close and destroy session
                            session.Close();
                        }
                    }
                }
            }
        }
        return lastHr;
    }
    return 0;
}
catch (const hresult_error& error)
{
    wprintf(error.message().c_str());
    return error.code();
}
catch (const std::exception& error)
{
    printf(error.what());
    return EXIT_FAILURE;
}
catch (...)
{
    printf("Unknown exception occurred.");
    return EXIT_FAILURE;
}
