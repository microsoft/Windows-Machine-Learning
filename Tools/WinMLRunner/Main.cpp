#include "Common.h"
#include "OutputHelper.h"
#include "ModelBinding.h"
#include "BindingUtilities.h"
#include "CommandLineArgs.h"
#include <filesystem>
#include <d3d11.h>
#include <Windows.Graphics.DirectX.Direct3D11.interop.h>

Profiler<WINML_MODEL_TEST_PERF> g_Profiler;

using namespace winrt::Windows::Graphics::DirectX::Direct3D11;

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

std::vector<ILearningModelFeatureValue> GenerateInputFeatures(
    const LearningModel& model,
    const CommandLineArgs& args,
    InputBindingType inputBindingType,
    InputDataType inputDataType,
    const IDirect3DDevice winrtDevice)
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
            auto imageFeature = BindingUtilities::CreateBindableImage(description, args.ImagePath(), inputBindingType, inputDataType, winrtDevice);
            inputFeatures.push_back(imageFeature);
        }
    }

    return inputFeatures;
}

HRESULT BindInputFeatures(const LearningModel& model, const LearningModelBinding& context, const std::vector<ILearningModelFeatureValue>& inputFeatures, const CommandLineArgs& args, OutputHelper& output, bool capturePerf)
{
    assert(model.InputFeatures().Size() == inputFeatures.size());

    try
    {
        context.Clear();

        Timer timer;

        if (capturePerf)
        {
            timer.Start();
            WINML_PROFILING_START(g_Profiler, WINML_MODEL_TEST_PERF::BIND_VALUE);
        }

        for (uint32_t i = 0; i < model.InputFeatures().Size(); i++)
        {
            auto&& description = model.InputFeatures().GetAt(i);
            context.Bind(description.Name(), inputFeatures[i]);
        }

        if (capturePerf)
        {
            WINML_PROFILING_STOP(g_Profiler, WINML_MODEL_TEST_PERF::BIND_VALUE);
            output.m_clockBindTimes.push_back(timer.Stop());
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

HRESULT EvaluateModel(
    const LearningModel& model,
    const LearningModelBinding& context,
    LearningModelSession& session,
    bool isGarbageData,
    const CommandLineArgs& args,
    OutputHelper& output,
    bool capturePerf
)
{
    LearningModelEvaluationResult result = nullptr;

    try
    {
        // Timer measures wall-clock time between the last two start/stop calls.
        Timer timer;

        if (capturePerf)
        {
            timer.Start();
            WINML_PROFILING_START(g_Profiler, WINML_MODEL_TEST_PERF::EVAL_MODEL);
        }

        result = session.Evaluate(context, L"");

        if (capturePerf)
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
    InputDataType inputDataType,
    DeviceCreationLocation deviceCreationLocation
)
{
    if (model == nullptr)
    {
        return hresult_invalid_argument().code();
    }

    LearningModelSession session = nullptr;
    IDirect3DDevice winrtDevice = nullptr;

    try
    {
        if (deviceCreationLocation == DeviceCreationLocation::ClientCode)
        {
            // Creating the device on the client and using it to create the video frame and initialize the session makes sure that everything is on
            // the same device. This usually avoids an expensive cross-device and cross-videoframe copy via the VideoFrame pipeline.
            com_ptr<ID3D11Device> d3d11Device;
            HRESULT hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, D3D11_CREATE_DEVICE_BGRA_SUPPORT, nullptr, 0, D3D11_SDK_VERSION, d3d11Device.put(), nullptr, nullptr);

            if (FAILED(hr))
            {
                throw hresult(hr);
            }

            com_ptr<IDXGIDevice> dxgiDevice;
            hr = d3d11Device->QueryInterface(IID_PPV_ARGS(dxgiDevice.put()));

            if (FAILED(hr))
            {
                throw hresult(hr);
            }

            com_ptr<IInspectable> inspectableDevice;
            hr = CreateDirect3D11DeviceFromDXGIDevice(dxgiDevice.get(), inspectableDevice.put());

            if (FAILED(hr))
            {
                throw hresult(hr);
            }

            winrtDevice = inspectableDevice.as<IDirect3DDevice>();
            LearningModelDevice learningModelDevice = LearningModelDevice::CreateFromDirect3D11Device(winrtDevice);
            session = LearningModelSession(model, learningModelDevice);
        }
        else
        {
            session = LearningModelSession(model, TypeHelper::GetWinmlDeviceKind(deviceType));
        }
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
        bool captureIterationPerf = args.PerfCapture() && (!args.IgnoreFirstRun() || i > 0);

        output.PrintBindingInfo(i + 1, deviceType, inputBindingType, inputDataType, deviceCreationLocation);

        std::vector<ILearningModelFeatureValue> inputFeatures = GenerateInputFeatures(model, args, inputBindingType, inputDataType, winrtDevice);
        HRESULT bindInputResult = BindInputFeatures(model, context, inputFeatures, args, output, captureIterationPerf);

        if (FAILED(bindInputResult))
        {
            return bindInputResult;
        }

        output.PrintEvaluatingInfo(i + 1, deviceType, inputBindingType, inputDataType, deviceCreationLocation);

        HRESULT evalResult = EvaluateModel(model, context, session, isGarbageData, args, output, captureIterationPerf);

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
    const std::vector<DeviceCreationLocation> deviceCreationLocations,
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
                    for (auto deviceCreationLocation : deviceCreationLocations)
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

                        HRESULT evalHResult = EvaluateModel(model, args, output, deviceType, inputBindingType, inputDataType, deviceCreationLocation);

                        if (FAILED(evalHResult))
                        {
                            return evalHResult;
                        }

                        if (args.PerfCapture())
                        {
                            output.PrintResults(g_Profiler, args.NumIterations(), deviceType, inputBindingType, inputDataType, deviceCreationLocation);
                            output.WritePerformanceDataToCSV(
                                g_Profiler,
                                args.NumIterations(),
                                path,
                                TypeHelper::Stringify(deviceType),
                                TypeHelper::Stringify(inputDataType),
                                TypeHelper::Stringify(inputBindingType),
                                TypeHelper::Stringify(deviceCreationLocation),
                                args.IgnoreFirstRun()
                            );
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

std::vector<DeviceCreationLocation> FetchDeviceCreationLocations(const CommandLineArgs& args)
{
    std::vector<DeviceCreationLocation> deviceCreationLocations;

    if (args.CreateDeviceInWinML())
    {
        deviceCreationLocations.push_back(DeviceCreationLocation::WinML);
    }

    if (args.CreateDeviceOnClient())
    {
        deviceCreationLocations.push_back(DeviceCreationLocation::ClientCode);
    }

    return deviceCreationLocations;
}

int main(int argc, char** argv)
{
    // Initialize COM in a multi-threaded environment.
    winrt::init_apartment();

    CommandLineArgs args;
    OutputHelper output(args.Silent());

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

    if (!args.ModelPath().empty() || !args.FolderPath().empty())
    {
        std::vector<DeviceType> deviceTypes = FetchDeviceTypes(args);
        std::vector<InputBindingType> inputBindingTypes = FetchInputBindingTypes(args);
        std::vector<InputDataType> inputDataTypes = FetchInputDataTypes(args);
        std::vector<DeviceCreationLocation> deviceCreationLocations = FetchDeviceCreationLocations(args);
        std::vector<std::wstring> modelPaths = args.ModelPath().empty() ? GetModelsInDirectory(args, &output) : std::vector<std::wstring>(1, args.ModelPath());

        return EvaluateModels(modelPaths, deviceTypes, inputBindingTypes, inputDataTypes, deviceCreationLocations, args, output);
    }

    return 0;
}