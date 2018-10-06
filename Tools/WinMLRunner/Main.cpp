#include "Common.h"
#include "OutputHelper.h"
#include "ModelBinding.h"
#include "BindingUtilities.h"
#include "CommandLineArgs.h"
#include <filesystem>
#include <evntprov.h>

Profiler<WINML_MODEL_TEST_PERF> g_Profiler;

// Markers for easy profiling
REGHANDLE gStartInputBindingHandle;
REGHANDLE gStopInputBindingHandle;
REGHANDLE gStartOutputBindingHandle;
REGHANDLE gStopOutputBindingHandle;
REGHANDLE gStartEvaluatingHandle;
REGHANDLE gStopEvaluatingHandle;

LearningModel LoadModel(const std::wstring path, bool capturePerf, bool silent, OutputHelper& output)
{
    Timer timer;
    LearningModel model = nullptr;

    output.PrintLoadingInfo(path);

    try
    {
        if (capturePerf)
        {
            WINML_PROFILING_START(g_Profiler, WINML_MODEL_TEST_PERF::LOAD_MODEL);
            timer.Start();
        }
        model = LearningModel::LoadFromFilePath(path);

        if (capturePerf)
        {
            WINML_PROFILING_STOP(g_Profiler, WINML_MODEL_TEST_PERF::LOAD_MODEL);
            output.m_clockLoadTime = timer.Stop();
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

std::vector<ILearningModelFeatureValue> GenerateInputFeatures(const LearningModel& model, const CommandLineArgs& args, InputBindingType inputBindingType, InputDataType inputDataType)
{
    std::vector<ILearningModelFeatureValue> inputFeatures;

    for (uint32_t i = 0; i < model.InputFeatures().Size(); i++)
    {
        auto&& description = model.InputFeatures().GetAt(i);

        if (inputDataType == InputDataType::Tensor || i > 0)
        {
            // For now, only the first input can be bound with real data
            std::wstring csvPath = i == 0 ? args.CsvPath() : std::wstring();
            auto tensorFeature = BindingUtilities::CreateBindableTensor(description, csvPath);
            inputFeatures.push_back(tensorFeature);
        }
        else
        {
            auto imageFeature = BindingUtilities::CreateBindableImage(description, args.ImagePath(), inputBindingType, inputDataType);
            inputFeatures.push_back(imageFeature);
        }
    }

    return inputFeatures;
}

HRESULT BindInputFeatures(const LearningModel& model, const LearningModelBinding& context, const std::vector<ILearningModelFeatureValue>& inputFeatures, const CommandLineArgs& args, OutputHelper& output)
{
    assert(model.InputFeatures().Size() == inputFeatures.size());

    try
    {
        context.Clear();

        if (args.UseMarkers())
        {
            EventWriteString(gStartInputBindingHandle, 0, 0, L"Start Input Binding");
        }

        Timer timer;

        if (args.PerfCapture())
        {
            timer.Start();
            WINML_PROFILING_START(g_Profiler, WINML_MODEL_TEST_PERF::BIND_VALUE);
        }

        for (uint32_t i = 0; i < model.InputFeatures().Size(); i++)
        {
            auto&& description = model.InputFeatures().GetAt(i);
            context.Bind(description.Name(), inputFeatures[i]);
        }

        if (args.PerfCapture())
        {
            WINML_PROFILING_STOP(g_Profiler, WINML_MODEL_TEST_PERF::BIND_VALUE);
            output.m_clockBindTimes.push_back(timer.Stop());
        }

        if (args.UseMarkers())
        {
            EventWriteString(gStopInputBindingHandle, 0, 0, L"Stop Input Binding");
        }

        if (!args.Silent())
        {
            std::cout << "[SUCCESS]" << std::endl;
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

// TODO:pavignol Refactor
HRESULT EvaluateModel(
    const LearningModel& model,
    const LearningModelBinding& context,
    LearningModelSession& session,
    bool isGarbageData,
    const CommandLineArgs& args,
    OutputHelper& output
)
{
    if (args.UseMarkers())
    {
        EventWriteString(gStartEvaluatingHandle, 0, 0, L"Start Evaluating");
    }

    LearningModelEvaluationResult result = nullptr;

    try
    {
        // Timer measures wall-clock time between the last two start/stop calls.
        Timer timer;

        if (args.PerfCapture())
        {
            timer.Start();
            WINML_PROFILING_START(g_Profiler, WINML_MODEL_TEST_PERF::EVAL_MODEL);
        }

        result = session.Evaluate(context, L"");

        if (args.PerfCapture())
        {
            WINML_PROFILING_STOP(g_Profiler, WINML_MODEL_TEST_PERF::EVAL_MODEL);
            output.m_clockEvalTimes.push_back(timer.Stop());
        }
    }
    catch (winrt::hresult_error hr)
    {
        std::cout << "[FAILED]" << std::endl;
        std::wcout << hr.message().c_str() << std::endl;
        return hr.code();
    }

    if (!args.Silent())
    {
        std::cout << "[SUCCESS]" << std::endl;
    }

    if (args.UseMarkers())
    {
        EventWriteString(gStopEvaluatingHandle, 0, 0, L"Stop Evaluating");
    }

    if (!isGarbageData && !args.Silent())
    {
        BindingUtilities::PrintEvaluationResults(model, args, result.Outputs());
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
    InputDataType inputDataType
)
{
    if (model == nullptr)
    {
        return hresult_invalid_argument().code();
    }
    LearningModelSession session = nullptr;

    try
    {
        session = LearningModelSession(model, TypeHelper::GetWinmlDeviceKind(deviceType));
    }
    catch (hresult_error hr)
    {
        std::cout << "Creating session [FAILED]" << std::endl;
        std::wcout << hr.message().c_str() << std::endl;
        return hr.code();
    }

    if (args.EnableDebugOutput())
    {
        // Enables trace log output. 
        session.EvaluationProperties().Insert(L"EnableDebugOutput", nullptr);
    }

    LearningModelBinding context(session);

    bool useInputData = false;
    
    // Add one more iteration if we ignore the first run
    uint32_t numIterations = args.NumIterations() + args.IgnoreFirstRun();

    bool isGarbageData = !args.CsvPath().empty() || !args.ImagePath().empty();

    // Run the binding + evaluate multiple times and average the results
    for (uint32_t i = 0; i < numIterations; i++)
    {
        bool capturePerf = args.PerfCapture() && (!args.IgnoreFirstRun() || i > 0);

        output.PrintBindingInfo(i + 1, deviceType, inputBindingType, inputDataType);

        std::vector<ILearningModelFeatureValue> inputFeatures = GenerateInputFeatures(model, args, inputBindingType, inputDataType);
        HRESULT bindInputResult = BindInputFeatures(model, context, inputFeatures, args, output);

        if (FAILED(bindInputResult))
        {
            return bindInputResult;
        }

        output.PrintEvaluatingInfo(i + 1, deviceType, inputBindingType, inputDataType);

        HRESULT evalResult = EvaluateModel(model, context, session, isGarbageData, args, output);

        if (FAILED(evalResult))
        {
            return evalResult;
        }
    }

    session.Close();

    return S_OK;
}

HRESULT EvaluateModels(
    std::vector<std::wstring>& modelPaths,
    const std::vector<DeviceType>& deviceTypes,
    const std::vector<InputBindingType>& inputBindingTypes,
    const std::vector<InputDataType>& inputDataTypes,
    const CommandLineArgs& args,
    OutputHelper& output
)
{
    output.PrintHardwareInfo();

    for (std::wstring& path : modelPaths)
    {
        LearningModel model = nullptr;

        try
        {
            model = LoadModel(path, args.PerfCapture(), args.Silent(), output);
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
            continue;
        }

        for (auto deviceType : deviceTypes)
        {
            for (auto inputBindingType : inputBindingTypes)
            {
                for (auto inputDataType : inputDataTypes)
                {
                    if (args.PerfCapture())
                    {
                        output.Reset();
                        g_Profiler.Reset();
                    }

                    if (inputDataType != InputDataType::Tensor)
                    {
                        // Currently GPU binding only work with 4D tensors and RGBA/BGRA images
                        if (tensorDescriptor.Shape().Size() != 4 || tensorDescriptor.Shape().GetAt(1) != 3)
                        {
                            continue;
                        }
                    }

                    HRESULT evalHResult = EvaluateModel(model, args, output, deviceType, inputBindingType, inputDataType);

                    // Some models may fail because of bugs in the winml code or the model file itself, but we should still run the remaining models
                    if (FAILED(evalHResult))
                    {
                        return evalHResult;
                    }

                    if (args.PerfCapture())
                    {
                        output.PrintResults(g_Profiler, args.NumIterations(), deviceType, inputBindingType, inputDataType);
                        output.WritePerformanceDataToCSV(g_Profiler, args.NumIterations(), path, TypeHelper::Stringify(deviceType), TypeHelper::Stringify(inputDataType), TypeHelper::Stringify(inputBindingType), args.IgnoreFirstRun());
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

    if (!args.UseBGR() && !args.UseRGB())
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

    if (args.UseGPUHighPerformance())
    {
        deviceTypes.push_back(DeviceType::HighPerfGPU);
    }

    if (args.UseGPUMinPower())
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

    if (args.UseGPUBoundInput())
    {
        inputBindingTypes.push_back(InputBindingType::GPU);
    }

    return inputBindingTypes;
}

// Register the custom marker events for perf analysis
void RegisterCustomMarkers()
{
    GUID guid;

    UuidFromString((RPC_WSTR)L"3545d8c3-3bcb-4865-98a7-90efff40f22d", &guid);
    EventRegister(&guid, nullptr, nullptr, &gStartInputBindingHandle);

    UuidFromString((RPC_WSTR)L"06ecb327-079d-45b2-900b-68885ce32605", &guid);
    EventRegister(&guid, nullptr, nullptr, &gStopInputBindingHandle);

    UuidFromString((RPC_WSTR)L"5fec8460-02d6-4dff-bb65-c6c0a862d2f5", &guid);
    EventRegister(&guid, nullptr, nullptr, &gStartEvaluatingHandle);

    UuidFromString((RPC_WSTR)L"f4960254-f367-43d5-b324-b552fae1d9d8", &guid);
    EventRegister(&guid, nullptr, nullptr, &gStopEvaluatingHandle);
}

// Unregister the custom marker events for perf analysis
void UnregisterCustomMarkers()
{
    EventUnregister(gStartInputBindingHandle);
    EventUnregister(gStopInputBindingHandle);
    EventUnregister(gStartEvaluatingHandle);
    EventUnregister(gStopEvaluatingHandle);
}

int main(int argc, char** argv)
{
    // Initialize COM in a multi-threaded environment.
    winrt::init_apartment();

    CommandLineArgs args;
    OutputHelper output(args.Silent());

    if (args.UseMarkers())
    {
        RegisterCustomMarkers();
    }

    // Profiler is a wrapper class that captures and stores timing and memory usage data on the
    // CPU and GPU.
    g_Profiler.Enable();

    if (!args.OutputPath().empty())
    {
        output.SetCSVFileName(args.OutputPath());
    }
    else
    {
        output.SetDefaultCSVFileName();
    }
    
    std::vector<DeviceType> deviceTypes = FetchDeviceTypes(args);
    std::vector<InputBindingType> inputBindingTypes = FetchInputBindingTypes(args);
    std::vector<InputDataType> inputDataTypes = FetchInputDataTypes(args);

    if (!args.ModelPath().empty())
    {
        std::vector<std::wstring> modelPaths(1, args.ModelPath());
        return EvaluateModels(modelPaths, deviceTypes, inputBindingTypes, inputDataTypes, args, output);
    }
    else if (!args.FolderPath().empty())
    {
        std::vector<std::wstring> modelPaths = GetModelsInDirectory(args, &output);
        return EvaluateModels(modelPaths, deviceTypes, inputBindingTypes, inputDataTypes, args, output);
    }

    if (args.UseMarkers())
    {
        UnregisterCustomMarkers();
    }

    return 0;
}