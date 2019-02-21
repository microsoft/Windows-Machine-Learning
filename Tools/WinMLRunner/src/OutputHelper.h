#pragma once
#include "Common.h"
#include "CommandLineArgs.h"
#include <fstream>
#include <ctime>
#include <locale>
#include <utility>
#include <codecvt>
#include <iomanip>
#include <dxgi.h>
#include <Windows.Graphics.DirectX.Direct3D11.interop.h>
#include <direct.h>

using namespace winrt::Windows::AI::MachineLearning;
using namespace winrt::Windows::Storage::Streams;
using namespace ::Windows::Graphics::DirectX::Direct3D11;
using namespace winrt::Windows::Graphics::DirectX::Direct3D11;

// Stores performance information and handles output to the command line and CSV files.
class OutputHelper
{
public:
    OutputHelper(int numIterations)
    {
        m_clockLoadTimes.resize(numIterations, 0.0);
        m_clockBindTimes.resize(numIterations, 0.0);
        m_clockEvalTimes.resize(numIterations, 0.0);
        m_CPUWorkingDiff.resize(numIterations, 0.0);
        m_CPUWorkingStart.resize(numIterations, 0.0);
        m_GPUSharedDiff.resize(numIterations, 0.0);
        m_GPUDedicatedDiff.resize(numIterations, 0.0);
        m_GPUSharedStart.resize(numIterations, 0.0);
        m_outputResult.resize(numIterations, "");
        m_outputTensorHash.resize(numIterations, 0);
    }

    void PrintLoadingInfo(const std::wstring& modelPath) const
    {
        wprintf(L"Loading model (path = %s)...\n", modelPath.c_str());
    }

    void PrintBindingInfo(uint32_t iteration, DeviceType deviceType, InputBindingType inputBindingType, InputDataType inputDataType, DeviceCreationLocation deviceCreationLocation, const std::string& status) const
    {
        printf(
            "Binding (device = %s, iteration = %d, inputBinding = %s, inputDataType = %s, deviceCreationLocation = %s)...%s\n",
            TypeHelper::Stringify(deviceType).c_str(),
            iteration,
            TypeHelper::Stringify(inputBindingType).c_str(),
            TypeHelper::Stringify(inputDataType).c_str(),
            TypeHelper::Stringify(deviceCreationLocation).c_str(),
            status.c_str()
        );
    }

    void PrintEvaluatingInfo(uint32_t iteration, DeviceType deviceType, InputBindingType inputBindingType, InputDataType inputDataType, DeviceCreationLocation deviceCreationLocation, const std::string &status) const
    {
        printf(
            "Evaluating (device = %s, iteration = %d, inputBinding = %s, inputDataType = %s, deviceCreationLocation = %s)...%s\n",
            TypeHelper::Stringify(deviceType).c_str(),
            iteration,
            TypeHelper::Stringify(inputBindingType).c_str(),
            TypeHelper::Stringify(inputDataType).c_str(),
            TypeHelper::Stringify(deviceCreationLocation).c_str(),
            status.c_str()
        );
    }

    void PrintModelInfo(std::wstring modelPath, LearningModel model) const
    {
        std::cout << "=================================================================" << std::endl;
        std::wcout << "Name: " << model.Name().c_str() << std::endl;
        std::wcout << "Author: " << model.Author().c_str() << std::endl;
        std::wcout << "Version: " << model.Version() << std::endl;
        std::wcout << "Domain: " << model.Domain().c_str() << std::endl;
        std::wcout << "Description: " << model.Description().c_str() << std::endl;
        std::wcout << "Path: " << modelPath << std::endl;
        std::cout << "Support FP16: " << std::boolalpha << doesModelContainFP16(model) << std::endl;

        std::cout << std::endl;
        //print out information about input of model
        std::cout << "Input Feature Info:" << std::endl;
        for (auto&& inputFeature : model.InputFeatures())
        {
            PrintFeatureDescriptorInfo(inputFeature);
        }
        //print out information about output of model
        std::cout << "Output Feature Info:" << std::endl;
        for (auto&& outputFeature : model.OutputFeatures())
        {
            PrintFeatureDescriptorInfo(outputFeature);
        }
        std::cout << "=================================================================" << std::endl;
        std::cout << std::endl;
    }

    void PrintFeatureDescriptorInfo(const ILearningModelFeatureDescriptor &descriptor) const
    {
        //IMPORTANT: This learningModelFeatureKind array needs to match the "enum class 
        //LearningModelFeatureKind" idl in Windows.AI.MachineLearning.0.h
        const std::string learningModelFeatureKind[] =
        {
            "Tensor",
            "Sequence",
            "Map",
            "Image",
        };
        std::wstring name(descriptor.Name());
        std::wcout << "Name: " << name << std::endl;
        std::wcout << "Feature Kind: " << FeatureDescriptorToString(descriptor) << std::endl;
        std::cout << std::endl;
    }

    void PrintHardwareInfo() const
    {
        std::cout << "WinML Runner" << std::endl;

        com_ptr<IDXGIFactory4> factory;
        HRESULT hr;

        try
        {
            hr = CreateDXGIFactory2SEH(factory.put_void());
        }
        catch (...)
        {
            hr = E_FAIL;
        }
        if (hr != S_OK)
        {
            return;
        }

        //Print All Adapters
        com_ptr<IDXGIAdapter> adapter;
        for (UINT i = 0; ; ++i)
        {
            com_ptr<IDXGIAdapter1> spAdapter;
            if (factory->EnumAdapters1(i, spAdapter.put()) != S_OK)
            {
                break;
            }
            DXGI_ADAPTER_DESC1 pDesc;
            spAdapter->GetDesc1(&pDesc);
            printf("Index: %d, Description: %ls\n", i, pDesc.Description);
        }
    }

    void PrintLearningModelDevice(DeviceType deviceType, const LearningModelDevice& device)
    {
        if (deviceType == DeviceType::CPU)
        {
            std::cout << "\nCreating Session with CPU device" << std::endl;
            return;
        }

        IDirect3DDevice d3dDevice = device.Direct3D11Device();
        com_ptr<IDirect3DDxgiInterfaceAccess> dxgi;
        dxgi = d3dDevice.try_as<IDirect3DDxgiInterfaceAccess>();
        if (dxgi)
        {
            com_ptr<IDXGIDevice> dxgiDevice;
            dxgi->GetInterface(__uuidof(IDXGIDevice), dxgiDevice.put_void());
            com_ptr<IDXGIAdapter> adapter;
            dxgiDevice->GetAdapter(adapter.put());
            DXGI_ADAPTER_DESC description;
            if (SUCCEEDED(adapter->GetDesc(&description)))
            {
                std::wcout << L"\nCreating Session with GPU: " << description.Description << std::endl;
            }
        }
        else
        {
            std::cout << "Failed to Print Learning Model Device Information" << std::endl;
        }
    }

    void PrintResults(
        const Profiler<WINML_MODEL_TEST_PERF> &profiler,
        uint32_t numIterations,
        DeviceType deviceType,
        InputBindingType inputBindingType,
        InputDataType inputDataType,
        DeviceCreationLocation deviceCreationLocation,
        bool isPerformanceConsoleOutputVerbose
    ) const
    {
        double loadTime = profiler[LOAD_MODEL].GetAverage(CounterType::TIMER);
        double createSessionTime = profiler[CREATE_SESSION].GetAverage(CounterType::TIMER);

        double averageBindTime = profiler[BIND_VALUE].GetAverage(CounterType::TIMER);
        double minBindTime = profiler[BIND_VALUE].GetMin(CounterType::TIMER);
        double maxBindTime = profiler[BIND_VALUE].GetMax(CounterType::TIMER);
        double firstBindTime = profiler[BIND_VALUE_FIRST_RUN].GetAverage(CounterType::TIMER);

        double averageEvalTime = profiler[EVAL_MODEL].GetAverage(CounterType::TIMER);
        double minEvalTime = profiler[EVAL_MODEL].GetMin(CounterType::TIMER);
        double maxEvalTime = profiler[EVAL_MODEL].GetMax(CounterType::TIMER);
        double firstEvalTime = profiler[EVAL_MODEL_FIRST_RUN].GetAverage(CounterType::TIMER);

        double firstLoadWorkingSetMemoryUsage = profiler[LOAD_MODEL].GetAverage(CounterType::WORKING_SET_USAGE);
        double firstLoadSharedMemoryUsage = profiler[LOAD_MODEL].GetAverage(CounterType::GPU_SHARED_MEM_USAGE);
        double firstLoadDedicatedMemoryUsage = profiler[LOAD_MODEL].GetAverage(CounterType::GPU_DEDICATED_MEM_USAGE);
        double firstLoadPeakWorkingSetUsage = profiler[LOAD_MODEL].GetAverage(CounterType::PEAK_WORKING_SET_USAGE);

        double firstSessionCreationWorkingSetMemoryUsage = profiler[CREATE_SESSION].GetAverage(CounterType::WORKING_SET_USAGE);
        double firstSessionCreationSharedMemoryUsage = profiler[CREATE_SESSION].GetAverage(CounterType::GPU_SHARED_MEM_USAGE);
        double firstSessionCreationDedicatedMemoryUsage = profiler[CREATE_SESSION].GetAverage(CounterType::GPU_DEDICATED_MEM_USAGE);
        double firstSessionPeakWorkingSetUsage = profiler[CREATE_SESSION].GetAverage(CounterType::PEAK_WORKING_SET_USAGE);

        double averageBindMemoryUsage = profiler[BIND_VALUE].GetAverage(CounterType::WORKING_SET_USAGE);
        double minBindMemoryUsage = profiler[BIND_VALUE].GetMin(CounterType::WORKING_SET_USAGE);
        double maxBindMemoryUsage = profiler[BIND_VALUE].GetMax(CounterType::WORKING_SET_USAGE);
        double firstBindMemoryUsage = profiler[BIND_VALUE_FIRST_RUN].GetAverage(CounterType::WORKING_SET_USAGE);
        double firstBindPeakMemoryUsage = profiler[BIND_VALUE_FIRST_RUN].GetAverage(CounterType::PEAK_WORKING_SET_USAGE);

        double averageEvalMemoryUsage = profiler[EVAL_MODEL].GetAverage(CounterType::WORKING_SET_USAGE);
        double minEvalMemoryUsage = profiler[EVAL_MODEL].GetMin(CounterType::WORKING_SET_USAGE);
        double maxEvalMemoryUsage = profiler[EVAL_MODEL].GetMax(CounterType::WORKING_SET_USAGE);
        double firstEvalMemoryUsage = profiler[EVAL_MODEL_FIRST_RUN].GetAverage(CounterType::WORKING_SET_USAGE);
        double firstEvalPeakMemoryUsage = profiler[EVAL_MODEL_FIRST_RUN].GetAverage(CounterType::PEAK_WORKING_SET_USAGE);

        double averageBindDedicatedMemoryUsage = profiler[BIND_VALUE].GetAverage(CounterType::GPU_DEDICATED_MEM_USAGE);
        double minBindDedicatedMemoryUsage = profiler[BIND_VALUE].GetMin(CounterType::GPU_DEDICATED_MEM_USAGE);
        double maxBindDedicatedMemoryUsage = profiler[BIND_VALUE].GetMax(CounterType::GPU_DEDICATED_MEM_USAGE);
        double firstBindDedicatedMemoryUsage = profiler[BIND_VALUE_FIRST_RUN].GetAverage(CounterType::GPU_DEDICATED_MEM_USAGE);

        double averageEvalDedicatedMemoryUsage = profiler[EVAL_MODEL].GetAverage(CounterType::GPU_DEDICATED_MEM_USAGE);
        double minEvalDedicatedMemoryUsage = profiler[EVAL_MODEL].GetMin(CounterType::GPU_DEDICATED_MEM_USAGE);
        double maxEvalDedicatedMemoryUsage = profiler[EVAL_MODEL].GetMax(CounterType::GPU_DEDICATED_MEM_USAGE);
        double firstEvalDedicatedMemoryUsage = profiler[EVAL_MODEL_FIRST_RUN].GetAverage(CounterType::GPU_DEDICATED_MEM_USAGE);

        double averageBindSharedMemoryUsage = profiler[BIND_VALUE].GetAverage(CounterType::GPU_SHARED_MEM_USAGE);
        double minBindSharedMemoryUsage = profiler[BIND_VALUE].GetMin(CounterType::GPU_SHARED_MEM_USAGE);
        double maxBindSharedMemoryUsage = profiler[BIND_VALUE].GetMax(CounterType::GPU_SHARED_MEM_USAGE);
        double firstBindSharedMemoryUsage = profiler[BIND_VALUE_FIRST_RUN].GetAverage(CounterType::GPU_SHARED_MEM_USAGE);

        double averageEvalSharedMemoryUsage = profiler[EVAL_MODEL].GetAverage(CounterType::GPU_SHARED_MEM_USAGE);
        double minEvalSharedMemoryUsage = profiler[EVAL_MODEL].GetMin(CounterType::GPU_SHARED_MEM_USAGE);
        double maxEvalSharedMemoryUsage = profiler[EVAL_MODEL].GetMax(CounterType::GPU_SHARED_MEM_USAGE);
        double firstEvalSharedMemoryUsage = profiler[EVAL_MODEL_FIRST_RUN].GetAverage(CounterType::GPU_SHARED_MEM_USAGE);

        double firstIterationWorkingSetMemoryUsage =
            profiler[LOAD_MODEL].GetAverage(CounterType::WORKING_SET_USAGE) +
            profiler[CREATE_SESSION].GetAverage(CounterType::WORKING_SET_USAGE) +
            profiler[BIND_VALUE_FIRST_RUN].GetAverage(CounterType::WORKING_SET_USAGE) +
            profiler[EVAL_MODEL_FIRST_RUN].GetAverage(CounterType::WORKING_SET_USAGE);

        double firstIterationSharedMemoryUsage =
            profiler[LOAD_MODEL].GetAverage(CounterType::GPU_SHARED_MEM_USAGE) +
            profiler[CREATE_SESSION].GetAverage(CounterType::GPU_SHARED_MEM_USAGE) +
            profiler[BIND_VALUE_FIRST_RUN].GetAverage(CounterType::GPU_SHARED_MEM_USAGE) +
            profiler[EVAL_MODEL_FIRST_RUN].GetAverage(CounterType::GPU_SHARED_MEM_USAGE);

        double firstIterationDedicatedMemoryUsage =
            profiler[LOAD_MODEL].GetAverage(CounterType::GPU_DEDICATED_MEM_USAGE) +
            profiler[CREATE_SESSION].GetAverage(CounterType::GPU_DEDICATED_MEM_USAGE) +
            profiler[BIND_VALUE_FIRST_RUN].GetAverage(CounterType::GPU_DEDICATED_MEM_USAGE) +
            profiler[EVAL_MODEL_FIRST_RUN].GetAverage(CounterType::GPU_DEDICATED_MEM_USAGE);

        double firstIterationPeakWorkingSet = firstLoadPeakWorkingSetUsage + firstSessionPeakWorkingSetUsage + firstBindPeakMemoryUsage + firstEvalPeakMemoryUsage;

        printf("\nResults (device = %s, numIterations = %d, inputBinding = %s, inputDataType = %s, deviceCreationLocation = %s):\n",
            TypeHelper::Stringify(deviceType).c_str(),
            numIterations,
            TypeHelper::Stringify(inputBindingType).c_str(),
            TypeHelper::Stringify(inputDataType).c_str(),
            TypeHelper::Stringify(deviceCreationLocation).c_str()
        );

        std::cout << "\nFirst Iteration Performance (load, bind, session creation, and evaluate): " << std::endl;
        std::cout << "  Load: " << loadTime << " ms" << std::endl;
        std::cout << "  Bind: " << firstBindTime << " ms" << std::endl;
        std::cout << "  Session Creation: " << createSessionTime << " ms" << std::endl;
        std::cout << "  Evaluate: " << firstEvalTime << " ms" << std::endl;

        if (isPerformanceConsoleOutputVerbose)
        {
            std::cout << "\n  Working Set Memory usage (load): " << firstLoadWorkingSetMemoryUsage << " MB" << std::endl;
            std::cout << "  Working Set Memory usage (session creation): " << firstSessionCreationWorkingSetMemoryUsage << " MB" << std::endl;
            std::cout << "  Working Set Memory usage (bind): " << firstBindMemoryUsage << " MB" << std::endl;
        }
        else
        {
            std::cout << std::endl;
        }
        std::cout << "  Working Set Memory usage (evaluate): " << firstEvalMemoryUsage << " MB" << std::endl;
        std::cout << "  Working Set Memory usage (load, bind, session creation, and evaluate): " << firstIterationWorkingSetMemoryUsage << " MB" << std::endl;

        if (isPerformanceConsoleOutputVerbose)
        {
            std::cout << "  Peak Working Set Memory Difference (from start to load): " << firstLoadPeakWorkingSetUsage << " MB" << std::endl;
            std::cout << "  Peak Working Set Memory Difference (from model load to session creation): " << firstSessionPeakWorkingSetUsage << " MB" << std::endl;
            std::cout << "  Peak Working Set Memory Difference (from session to bind): " << firstBindPeakMemoryUsage << " MB" << std::endl;
            std::cout << "  Peak Working Set Memory Difference (from bind to evaluate): " << firstEvalPeakMemoryUsage << " MB" << std::endl;
        }

        std::cout << "  Peak Working Set Memory Difference (load, bind, session creation, and evaluate): " << firstIterationPeakWorkingSet << " MB" << std::endl;

        if (isPerformanceConsoleOutputVerbose)
        {
            std::cout << "\n  Dedicated Memory usage (load): " << firstLoadDedicatedMemoryUsage << " MB" << std::endl;
            std::cout << "  Dedicated Memory usage (session creation): " << firstSessionCreationDedicatedMemoryUsage << " MB" << std::endl;
            std::cout << "  Dedicated Memory usage (bind): " << firstBindDedicatedMemoryUsage << " MB" << std::endl;
        }
        else
        {
            std::cout << std::endl;
        }
        std::cout << "  Dedicated Memory usage (evaluate): " << firstEvalDedicatedMemoryUsage << " MB" << std::endl;
        std::cout << "  Dedicated Memory usage (load, bind, session creation, and evaluate): " << firstIterationDedicatedMemoryUsage << " MB" << std::endl;

        if (isPerformanceConsoleOutputVerbose)
        {
            std::cout << "\n  Shared Memory usage (load): " << firstLoadSharedMemoryUsage << " MB" << std::endl;
            std::cout << "  Shared Memory usage (session creation): " << firstSessionCreationSharedMemoryUsage << " MB" << std::endl;
            std::cout << "  Shared Memory usage (bind): " << firstBindSharedMemoryUsage << " MB" << std::endl;
        }
        else
        {
            std::cout << std::endl;
        }
        std::cout << "  Shared Memory usage (evaluate): " << firstEvalSharedMemoryUsage << " MB" << std::endl;
        std::cout << "  Shared Memory usage (load, bind, session creation, and evaluate): " << firstIterationSharedMemoryUsage << " MB" << std::endl;

        if (numIterations > 1)
        {
            printf("\nAverage Performance excluding first iteration. Iterations %d to %d. (Iterations greater than 1 only bind and evaluate)\n", 2, numIterations);
            std::cout << "  Average Bind: " << averageBindTime << " ms" << std::endl;
            if (isPerformanceConsoleOutputVerbose)
            {
                std::cout << "  Minimum Bind: " << minBindTime << " ms" << std::endl;
                std::cout << "  Maximum Bind: " << maxBindTime << " ms" << std::endl;
            }
            std::cout << "  Average Evaluate: " << averageEvalTime << " ms" << std::endl;
            if (isPerformanceConsoleOutputVerbose)
            {
                std::cout << "  Minimum Evaluate: " << minEvalTime << " ms" << std::endl;
                std::cout << "  Maximum Evaluate: " << maxEvalTime << " ms" << std::endl;
            }

            std::cout << "\n  Average Working Set Memory usage (bind): " << averageBindMemoryUsage << " MB" << std::endl;
            if (isPerformanceConsoleOutputVerbose)
            {
                std::cout << "  Min Working Set Memory usage (bind): " << minBindMemoryUsage << " MB" << std::endl;
                std::cout << "  Max Working Set Memory usage (bind): " << maxBindMemoryUsage << " MB" << std::endl;
            }
            std::cout << "  Average Working Set Memory usage (evaluate): " << averageEvalMemoryUsage << " MB" << std::endl;
            if (isPerformanceConsoleOutputVerbose)
            {
                std::cout << "  Min Working Set Memory usage (evaluate): " << minEvalMemoryUsage << " MB" << std::endl;
                std::cout << "  Max Working Set Memory usage (evaluate): " << maxEvalMemoryUsage << " MB" << std::endl;
            }

            std::cout << "\n  Average Dedicated Memory usage (bind): " << averageBindDedicatedMemoryUsage << " MB" << std::endl;
            if (isPerformanceConsoleOutputVerbose)
            {
                std::cout << "  Min Dedicated Memory usage (bind): " << minBindDedicatedMemoryUsage << " MB" << std::endl;
                std::cout << "  Max Dedicated Memory usage (bind): " << maxBindDedicatedMemoryUsage << " MB" << std::endl;
            }
            std::cout << "  Average Dedicated Memory usage (evaluate): " << averageEvalDedicatedMemoryUsage << " MB" << std::endl;
            if (isPerformanceConsoleOutputVerbose)
            {
                std::cout << "  Min Dedicated Memory usage (evaluate): " << minEvalDedicatedMemoryUsage << " MB" << std::endl;
                std::cout << "  Max Dedicated Memory usage (evaluate): " << maxEvalDedicatedMemoryUsage << " MB" << std::endl;
            }

            std::cout << "\n  Average Shared Memory usage (bind): " << averageBindSharedMemoryUsage << " MB" << std::endl;
            if (isPerformanceConsoleOutputVerbose)
            {
                std::cout << "  Min Shared Memory usage (bind): " << minBindSharedMemoryUsage << " MB" << std::endl;
                std::cout << "  Max Shared Memory usage (bind): " << maxBindSharedMemoryUsage << " MB" << std::endl;
            }
            std::cout << "  Average Shared Memory usage (evaluate): " << averageEvalSharedMemoryUsage << " MB" << std::endl;
            if (isPerformanceConsoleOutputVerbose)
            {
                std::cout << "  Min Shared Memory usage (evaluate): " << minEvalSharedMemoryUsage << " MB" << std::endl;
                std::cout << "  Max Shared Memory usage (evaluate): " << maxEvalSharedMemoryUsage << " MB" << std::endl;
            }
        }
        std::cout << std::endl << std::endl << std::endl;
    }

    static std::wstring FeatureDescriptorToString(const ILearningModelFeatureDescriptor &descriptor)
    {
        //IMPORTANT: This tensorKinds array needs to match the "enum class TensorKind" idl in Windows.AI.MachineLearning.0.h
        const std::wstring tensorKind[] =
        {
            L"Undefined",
            L"Float",
            L"UInt8",
            L"Int8",
            L"UInt16",
            L"Int16",
            L"Int32",
            L"Int64",
            L"String",
            L"Boolean",
            L"Float16",
            L"Double",
            L"UInt32",
            L"UInt64",
            L"Complex64",
            L"Complex128",
        };
        switch (descriptor.Kind())
        {
        case LearningModelFeatureKind::Tensor:
        {
            auto tensorDescriptor = descriptor.as<TensorFeatureDescriptor>();
            return tensorKind[(int)tensorDescriptor.TensorKind()];
        }
        case LearningModelFeatureKind::Image:
        {
            auto imageDescriptor = descriptor.as<ImageFeatureDescriptor>();
            std::wstring str = L"Image (Height: " + std::to_wstring(imageDescriptor.Height()) +
                               L", Width:  " + std::to_wstring(imageDescriptor.Width()) + L")";
            return str;
        }
        case LearningModelFeatureKind::Map:
        {
            auto mapDescriptor = descriptor.as<MapFeatureDescriptor>();
            std::wstring str = L"Map<" + tensorKind[(int)mapDescriptor.KeyKind()] + L",";
            str += FeatureDescriptorToString(mapDescriptor.ValueDescriptor());
            str += L">";
            return str;
        }
        case LearningModelFeatureKind::Sequence:
        {
            auto sequenceDescriptor = descriptor.as<SequenceFeatureDescriptor>();
            std::wstring str = L"List<" + FeatureDescriptorToString(sequenceDescriptor.ElementDescriptor()) + L">";
            return str;
        }
        default:
            return (L"Invalid feature %s.", descriptor.Name().c_str());
        }
    }

    static bool doesDescriptorContainFP16(const ILearningModelFeatureDescriptor &descriptor)
    {
        switch (descriptor.Kind())
        {
            case LearningModelFeatureKind::Tensor:
            {
                return descriptor.as<TensorFeatureDescriptor>().TensorKind() == TensorKind::Float16;
            }
            break;
            case LearningModelFeatureKind::Map:
            {
                auto mapDescriptor = descriptor.as<MapFeatureDescriptor>();
                if (mapDescriptor.KeyKind() == TensorKind::Float16)
                {
                    return true;
                }
                return doesDescriptorContainFP16(mapDescriptor.ValueDescriptor());
            }
            break;
            case LearningModelFeatureKind::Sequence:
            {
                return doesDescriptorContainFP16(descriptor.as<SequenceFeatureDescriptor>().ElementDescriptor());
            }
            break;
            default:
            {
                return false;
            }
        }
    }

    static bool doesModelContainFP16(const LearningModel model)
    {
        for (auto&& inputFeature : model.InputFeatures())
        {
            if (doesDescriptorContainFP16(inputFeature)) {
                return true;
            }
        }
        return false;
    }

    void SaveLoadTimes(Profiler<WINML_MODEL_TEST_PERF> &profiler, uint32_t iterNum)
    {
        m_clockLoadTimes[iterNum] = profiler[LOAD_MODEL].GetClockTime();
    }
    void SaveBindTimes(Profiler<WINML_MODEL_TEST_PERF> &profiler, uint32_t iterNum)
    {
        m_clockBindTimes[iterNum] = profiler[BIND_VALUE].GetClockTime();
    }
    void SaveEvalPerformance(Profiler<WINML_MODEL_TEST_PERF> &profiler, uint32_t iterNum)
    {
        m_clockEvalTimes[iterNum] = profiler[EVAL_MODEL].GetClockTime();
        m_CPUWorkingDiff[iterNum] = profiler[EVAL_MODEL].GetCpuWorkingDiff();
        m_CPUWorkingStart[iterNum] = profiler[EVAL_MODEL].GetCpuWorkingStart();
        m_GPUSharedDiff[iterNum] = profiler[EVAL_MODEL].GetGpuSharedDiff();
        m_GPUSharedStart[iterNum] = profiler[EVAL_MODEL].GetGpuSharedStart();
        m_GPUDedicatedDiff[iterNum] = profiler[EVAL_MODEL].GetGpuDedicatedDiff();
    }

    void SaveResult(uint32_t iterationNum, std::string result, int hashcode)
    {
        m_outputResult[iterationNum] = result;
        m_outputTensorHash[iterationNum] = hashcode;
    }
    
    void SetDefaultPerIterationFolder(const std::wstring &folderName)
    {
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
        m_folderNamePerIteration = converter.to_bytes(folderName);
        if (_mkdir(m_folderNamePerIteration.c_str()) != 0)
            std::cout << "Folder cannot be created";
    }

    void SetDefaultCSVFileNamePerIteration()
    {
        std::string fileNamePerIteration = m_folderNamePerIteration + "\\Summary.csv";
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
        m_csvFileNamePerIterationSummary = converter.from_bytes(fileNamePerIteration);
    }

    void SetDefaultCSVIterationResult(uint32_t iterationNum, const CommandLineArgs &args, std::string &featureName)
    {
        if (args.UseCPU() && args.UseGPU())
        {
            if (!m_flagGpuDevice)
            {
                m_fileNameResultDevice = m_folderNamePerIteration + "\\" + featureName + "CpuIteration";
                if (iterationNum == args.NumIterations() || args.SaveTensorMode() == "First")
                {
                    m_flagGpuDevice = true;
                }
            }
            else
            {
                m_fileNameResultDevice = m_folderNamePerIteration + "\\" + featureName + "GpuIteration";
            }
        }
        else if (args.UseGPU())
        {
            m_fileNameResultDevice = m_folderNamePerIteration + "\\" + featureName + "GpuIteration";
        }
        else
        {
            m_fileNameResultDevice = m_folderNamePerIteration + "\\" + featureName + "CpuIteration";
        }
        std::string fileNamePerIterationResult = m_fileNameResultDevice + std::to_string(iterationNum) + ".csv";
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
        m_csvFileNamePerIterationResult = converter.from_bytes(fileNamePerIterationResult);
    }

    void SetDefaultCSVFileName()
    {
        auto time = std::time(nullptr);
        struct tm localTime;
        localtime_s(&localTime, &time);

        std::ostringstream oss;
        oss << std::put_time(&localTime, "%Y-%m-%d %H.%M.%S");
        std::string fileName = "WinML Runner [" + oss.str() + "].csv";
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
        m_csvFileName = converter.from_bytes(fileName);
    }

    void SetCSVFileName(const std::wstring& fileName)
    {
        m_csvFileName = fileName;
    }

    void WritePerIterationPerformance(const CommandLineArgs& args, std::wstring model, std::wstring img)
    {
        if (m_csvFileNamePerIterationSummary.length() > 0)
        {
            bool bNewFile = false;
            std::ifstream fin;
            fin.open(m_csvFileNamePerIterationSummary);
            std::filebuf* outbuf = fin.rdbuf();
            if (EOF == outbuf->sbumpc())
            {
                bNewFile = true;
            }
            fin.close();

            std::ofstream fout;
            fout.open(m_csvFileNamePerIterationSummary, std::ios_base::app);

            std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
            std::string modelName = converter.to_bytes(model);
            std::string imgName = converter.to_bytes(img);

            if (bNewFile)
            {
                if (args.IsPerIterationCapture())
                {
                    fout << "Model Name" << ","
                         << "Image Name" << ","
                         << "Iterations" << ","
                         << "Iteration Number " << ","
                         << "CPU Working Set Diff (MB)" << ","
                         << "CPU Working Set Start (MB)" << ","
                         << "GPU Shared Memory Diff (MB)" << ","
                         << "GPU Shared Memory Start (MB)" << ","
                         << "GPU Dedicated Memory Diff (MB)" << ","
                         << "Load (ms)" << ","
                         << "Bind (ms)" << ","
                         << "Evaluate (ms)" << "," ;

                    if (args.IsSaveTensor())
                    {
                        fout << "Result" << "," 
                             << "OutputTensorHash" << "," 
                             << "FileName";
                    }
                }

                else if (args.IsSaveTensor())
                {
                    fout << "Iteration Number" << ","
                         << "Result" << ","
                         << "OutputTensorHash" << ","
                         << "FileName"; 
                }
                fout << std::endl;
            }

            if (args.IsPerIterationCapture())
            {
                for (uint32_t i = 0; i < args.NumIterations(); i++)
                {
                    fout << modelName << ","
                        << imgName << ","
                        << args.NumIterations() << ","
                        << i + 1 << ","
                        << m_CPUWorkingDiff[i] << ","
                        << m_CPUWorkingStart[i] << ","
                        << m_GPUSharedDiff[i] << ","
                        << m_GPUSharedStart[i] << ","
                        << m_GPUDedicatedDiff[i] << ","
                        << m_clockLoadTimes[i] << ","
                        << m_clockBindTimes[i] << ","
                        << m_clockEvalTimes[i] << ",";

                    if (args.IsSaveTensor() && (args.SaveTensorMode() == "All" || (args.SaveTensorMode()=="First" && i==0)))
                    {
                        fout << m_outputResult[i] << "," 
                             << m_outputTensorHash[i] << "," 
                             << m_fileNameResultDevice + std::to_string(i + 1) + ".csv" << ",";
                    }
                    fout << std::endl;
                }
            }
           
            else if (args.IsSaveTensor())
            {
                for (uint32_t i = 0; i < args.NumIterations(); i++)
                {
                    fout << i+1 << "," 
                         << m_outputResult[i] << ","
                         << m_outputTensorHash[i] << ","
                         << m_fileNameResultDevice + std::to_string(i + 1) + ".csv" << std::endl;
                    if (args.SaveTensorMode() == "First" && i == 0)
                    {
                        break;
                    }
                }
            }
            fout.close();
        }
    }

    template<typename T>
    void WriteTensorResultToCSV(T* m_Res, uint32_t iterationNum, const CommandLineArgs &args, uint32_t &capacity, std::string &featureName)
    {
        if (args.SaveTensorMode() == "First" && iterationNum > 1)
        {
            return;
        }
        SetDefaultCSVIterationResult(iterationNum, args, featureName);
        if (m_csvFileNamePerIterationResult.length() > 0)
        {
            std::ofstream fout;
            fout.open(m_csvFileNamePerIterationResult, std::ios_base::app);
            fout << "Index" << "," << "Value" << std::endl;
            int size = capacity / sizeof(T);
            for (int i = 0; i < size; i++)
            {
                fout << i << "," << *(m_Res+i) << std::endl;
            }
            fout.close();
        }
    }

    void WritePerformanceDataToCSV(
        const Profiler<WINML_MODEL_TEST_PERF> &profiler,
        int numIterations, std::wstring model,
        const std::string& deviceType,
        const std::string& inputBinding,
        const std::string& inputType,
        const std::string& deviceCreationLocation
    ) const
    {
        double loadTime = profiler[LOAD_MODEL].GetAverage(CounterType::TIMER);
        double createSessionTime = profiler[CREATE_SESSION].GetAverage(CounterType::TIMER);

        double averageBindTime = profiler[BIND_VALUE].GetAverage(CounterType::TIMER);
        double minBindTime = profiler[BIND_VALUE].GetMin(CounterType::TIMER);
        double maxBindTime = profiler[BIND_VALUE].GetMax(CounterType::TIMER);
        double firstBindTime = profiler[BIND_VALUE_FIRST_RUN].GetAverage(CounterType::TIMER);

        double averageEvalTime = profiler[EVAL_MODEL].GetAverage(CounterType::TIMER);
        double minEvalTime = profiler[EVAL_MODEL].GetMin(CounterType::TIMER);
        double maxEvalTime = profiler[EVAL_MODEL].GetMax(CounterType::TIMER);
        double firstEvalTime = profiler[EVAL_MODEL_FIRST_RUN].GetAverage(CounterType::TIMER);

        double firstLoadWorkingSetMemoryUsage = profiler[LOAD_MODEL].GetAverage(CounterType::WORKING_SET_USAGE);
        double firstLoadSharedMemoryUsage = profiler[LOAD_MODEL].GetAverage(CounterType::GPU_SHARED_MEM_USAGE);
        double firstLoadDedicatedMemoryUsage = profiler[LOAD_MODEL].GetAverage(CounterType::GPU_DEDICATED_MEM_USAGE);

        double firstSessionCreationWorkingSetMemoryUsage = profiler[CREATE_SESSION].GetAverage(CounterType::WORKING_SET_USAGE);
        double firstSessionCreationSharedMemoryUsage = profiler[CREATE_SESSION].GetAverage(CounterType::GPU_SHARED_MEM_USAGE);
        double firstSessionCreationDedicatedMemoryUsage = profiler[CREATE_SESSION].GetAverage(CounterType::GPU_DEDICATED_MEM_USAGE);

        double averageBindMemoryUsage = profiler[BIND_VALUE].GetAverage(CounterType::WORKING_SET_USAGE);
        double minBindMemoryUsage = profiler[BIND_VALUE].GetMin(CounterType::WORKING_SET_USAGE);
        double maxBindMemoryUsage = profiler[BIND_VALUE].GetMax(CounterType::WORKING_SET_USAGE);
        double firstBindMemoryUsage = profiler[BIND_VALUE_FIRST_RUN].GetAverage(CounterType::WORKING_SET_USAGE);

        double averageEvalMemoryUsage = profiler[EVAL_MODEL].GetAverage(CounterType::WORKING_SET_USAGE);
        double minEvalMemoryUsage = profiler[EVAL_MODEL].GetMin(CounterType::WORKING_SET_USAGE);
        double maxEvalMemoryUsage = profiler[EVAL_MODEL].GetMax(CounterType::WORKING_SET_USAGE);
        double firstEvalMemoryUsage = profiler[EVAL_MODEL_FIRST_RUN].GetAverage(CounterType::WORKING_SET_USAGE);

        double averageBindDedicatedMemoryUsage = profiler[BIND_VALUE].GetAverage(CounterType::GPU_DEDICATED_MEM_USAGE);
        double minBindDedicatedMemoryUsage = profiler[BIND_VALUE].GetMin(CounterType::GPU_DEDICATED_MEM_USAGE);
        double maxBindDedicatedMemoryUsage = profiler[BIND_VALUE].GetMax(CounterType::GPU_DEDICATED_MEM_USAGE);
        double firstBindDedicatedMemoryUsage = profiler[BIND_VALUE_FIRST_RUN].GetAverage(CounterType::GPU_DEDICATED_MEM_USAGE);

        double averageEvalDedicatedMemoryUsage = profiler[EVAL_MODEL].GetAverage(CounterType::GPU_DEDICATED_MEM_USAGE);
        double minEvalDedicatedMemoryUsage = profiler[EVAL_MODEL].GetMin(CounterType::GPU_DEDICATED_MEM_USAGE);
        double maxEvalDedicatedMemoryUsage = profiler[EVAL_MODEL].GetMax(CounterType::GPU_DEDICATED_MEM_USAGE);
        double firstEvalDedicatedMemoryUsage = profiler[EVAL_MODEL_FIRST_RUN].GetAverage(CounterType::GPU_DEDICATED_MEM_USAGE);

        double averageBindSharedMemoryUsage = profiler[BIND_VALUE].GetAverage(CounterType::GPU_SHARED_MEM_USAGE);
        double minBindSharedMemoryUsage = profiler[BIND_VALUE].GetMin(CounterType::GPU_SHARED_MEM_USAGE);
        double maxBindSharedMemoryUsage = profiler[BIND_VALUE].GetMax(CounterType::GPU_SHARED_MEM_USAGE);
        double firstBindSharedMemoryUsage = profiler[BIND_VALUE_FIRST_RUN].GetAverage(CounterType::GPU_SHARED_MEM_USAGE);

        double averageEvalSharedMemoryUsage = profiler[EVAL_MODEL].GetAverage(CounterType::GPU_SHARED_MEM_USAGE);
        double minEvalSharedMemoryUsage = profiler[EVAL_MODEL].GetMin(CounterType::GPU_SHARED_MEM_USAGE);
        double maxEvalSharedMemoryUsage = profiler[EVAL_MODEL].GetMax(CounterType::GPU_SHARED_MEM_USAGE);
        double firstEvalSharedMemoryUsage = profiler[EVAL_MODEL_FIRST_RUN].GetAverage(CounterType::GPU_SHARED_MEM_USAGE);

        if (!m_csvFileName.empty())
        {
            // Check if header exists
            bool bNewFile = false;
            std::ifstream fin;
            fin.open(m_csvFileName);
            std::filebuf* outbuf = fin.rdbuf();
            if (EOF == outbuf->sbumpc())
            {
                bNewFile = true;
            }
            fin.close();

            std::ofstream fout;
            fout.open(m_csvFileName, std::ios_base::app);

            std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
            std::string modelName = converter.to_bytes(model);
       
            if (bNewFile)
            {
                fout << "model name" << ","
                     << "device type" << ","
                     << "input binding" << ","
                     << "input type" << ","
                     << "device creation location" << ","
                     << "iterations" << ","
                     << "load (ms)" << ","
                     << "session creation (ms)" << ","
                     << "first bind (ms)" << ","
                     << "average bind (ms)" << ","
                     << "min bind (ms)" << ","
                     << "max bind (ms)" << ","
                     << "first evaluate (ms)" << ","
                     << "average evaluate (ms)" << ","
                     << "min evaluate (ms)" << ","
                     << "max evaluate (ms)" << ","
                     << "load working set memory (MB)" << ","
                     << "session creation working set memory (MB)" << ","
                     << "first bind working set memory (MB)" << ","
                     << "bind average working set memory (MB)" << ","
                     << "bind max working set memory (MB)" << ","
                     << "bind min working set memory (MB)" << ","
                     << "first evaluate working set memory (MB)" << ","
                     << "evaluate average working set memory (MB)" << ","
                     << "evaluate max working set memory (MB)" << ","
                     << "evaluate min working set memory (MB)" << ","
                     << "load dedicated memory (MB)" << ","
                     << "session creation dedicated memory (MB)" << ","
                     << "first bind dedicated memory (MB)" << ","
                     << "bind average dedicated memory (MB)" << ","
                     << "bind max dedicated memory (MB)" << ","
                     << "bind min dedicated memory (MB)" << ","
                     << "first evaluate dedicated memory (MB)" << ","
                     << "evaluate average dedicated memory (MB)" << ","
                     << "evaluate max dedicated memory (MB)" << ","
                     << "evaluate min dedicated memory (MB)" << ","
                     << "load shared memory (MB)" << ","
                     << "session creation shared memory (MB)" << ","
                     << "first bind shared memory (MB)" << ","
                     << "bind average shared memory (MB)" << ","
                     << "bind max shared memory (MB)" << ","
                     << "bind min shared memory (MB)" << ","
                     << "first evaluate shared memory (MB)" << ","
                     << "evaluate average shared memory (MB)" << ","
                     << "evaluate max shared memory (MB)" << ","
                     << "evaluate min shared memory (MB)" << std::endl;
            }
            fout << modelName << ","
                 << deviceType << ","
                 << inputBinding << ","
                 << inputType << ","
                 << deviceCreationLocation << ","
                 << numIterations << ","
                 << loadTime << ","
                 << createSessionTime << ","
                 << firstBindTime << ","
                 << ( numIterations <= 1 ? 0 : averageBindTime ) << ","
                 << ( numIterations <= 1 ? 0 : minBindTime ) << ","
                 << ( numIterations <= 1 ? 0 : maxBindTime ) << ","
                 << firstEvalTime << ","
                 << ( numIterations <= 1 ? 0 : averageEvalTime ) << ","
                 << ( numIterations <= 1 ? 0 : minEvalTime ) << ","
                 << ( numIterations <= 1 ? 0 : maxEvalTime ) << ","
                 << firstLoadWorkingSetMemoryUsage << ","
                 << firstSessionCreationWorkingSetMemoryUsage << ","
                 << firstBindMemoryUsage << ","
                 << ( numIterations <= 1 ? 0 : averageBindMemoryUsage ) << ","
                 << ( numIterations <= 1 ? 0 : maxBindMemoryUsage ) << ","
                 << ( numIterations <= 1 ? 0 : minBindMemoryUsage ) << ","
                 << firstEvalMemoryUsage << ","
                 << ( numIterations <= 1 ? 0 : averageEvalMemoryUsage ) << ","
                 << ( numIterations <= 1 ? 0 : maxEvalMemoryUsage ) << ","
                 << ( numIterations <= 1 ? 0 : minEvalMemoryUsage ) << ","
                 << firstLoadDedicatedMemoryUsage << ","
                 << firstSessionCreationDedicatedMemoryUsage << ","
                 << firstBindDedicatedMemoryUsage << ","
                 << ( numIterations <= 1 ? 0 : averageBindDedicatedMemoryUsage ) << ","
                 << ( numIterations <= 1 ? 0 : maxBindDedicatedMemoryUsage ) << ","
                 << ( numIterations <= 1 ? 0 : minBindDedicatedMemoryUsage ) << ","
                 << firstEvalDedicatedMemoryUsage << ","
                 << ( numIterations <= 1 ? 0 : averageEvalDedicatedMemoryUsage ) << ","
                 << ( numIterations <= 1 ? 0 : maxEvalDedicatedMemoryUsage ) << ","
                 << ( numIterations <= 1 ? 0 : minEvalDedicatedMemoryUsage ) << ","
                 << firstLoadSharedMemoryUsage << ","
                 << firstSessionCreationSharedMemoryUsage << ","
                 << firstBindSharedMemoryUsage << ","
                 << ( numIterations <= 1 ? 0 : averageBindSharedMemoryUsage ) << ","
                 << ( numIterations <= 1 ? 0 : maxBindSharedMemoryUsage ) << ","
                 << ( numIterations <= 1 ? 0 : minBindSharedMemoryUsage ) << ","
                 << firstEvalSharedMemoryUsage << ","
                 << ( numIterations <= 1 ? 0 : averageEvalSharedMemoryUsage ) << ","
                 << ( numIterations <= 1 ? 0 : maxEvalSharedMemoryUsage ) << ","
                 << ( numIterations <= 1 ? 0 : minEvalSharedMemoryUsage ) << "," << std::endl;
            fout.close();
        }
    }

    std::vector<double> m_clockLoadTimes;
    std::vector<double> m_clockBindTimes;
    std::vector<double> m_clockEvalTimes;

private:
    std::wstring m_csvFileName;
    std::wstring m_csvFileNamePerIterationSummary;
    std::wstring m_csvFileNamePerIterationResult;
    std::string m_folderNamePerIteration;
    std::string m_fileNameResultDevice;

    bool m_silent = false;
    bool m_flagGpuDevice = false;

    std::vector<double> m_EvalTime;
    std::vector<double> m_CPUWorkingDiff;
    std::vector<double> m_CPUWorkingStart;
    std::vector<double> m_GPUSharedDiff;
    std::vector<double> m_GPUSharedStart;
    std::vector<double> m_GPUDedicatedDiff;
    std::vector<std::string> m_outputResult;
    std::vector<int> m_outputTensorHash;
};