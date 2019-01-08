#pragma once
#include "Common.h"
#include "CommandLineArgs.h"
#include <fstream>
#include <ctime>
#include <locale>
#include <utility>
#include <codecvt>
#include <iomanip>

using namespace winrt::Windows::AI::MachineLearning;
using namespace winrt::Windows::Storage::Streams;

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

        com_ptr<IDXGIFactory6> factory;
        CreateDXGIFactory1(__uuidof(IDXGIFactory6), factory.put_void());
        com_ptr<IDXGIAdapter> adapter;
        factory->EnumAdapters(0, adapter.put());
        DXGI_ADAPTER_DESC description;
        if (SUCCEEDED(adapter->GetDesc(&description)))
        {
            std::wcout << L"GPU: " << description.Description << std::endl;
            std::cout << std::endl;
        }
    }

    void PrintResults(
        const Profiler<WINML_MODEL_TEST_PERF> &profiler,
        uint32_t numIterations,
        DeviceType deviceType,
        InputBindingType inputBindingType,
        InputDataType inputDataType,
        DeviceCreationLocation deviceCreationLocation
    ) const
    {
        double loadTime = profiler[LOAD_MODEL].GetAverage(CounterType::TIMER);
        double bindTime = profiler[BIND_VALUE].GetAverage(CounterType::TIMER);
        double evalTime = profiler[EVAL_MODEL].GetAverage(CounterType::TIMER);
        double evalMemoryUsage = profiler[EVAL_MODEL].GetAverage(CounterType::WORKING_SET_USAGE);
        double gpuEvalSharedMemoryUsage = profiler[EVAL_MODEL].GetAverage(CounterType::GPU_SHARED_MEM_USAGE);
        double gpuEvalDedicatedMemoryUsage = profiler[EVAL_MODEL].GetAverage(CounterType::GPU_DEDICATED_MEM_USAGE);

        double totalTime = (isnan(loadTime) ? 0 : loadTime) + bindTime + evalTime;

        std::cout << std::endl;

        printf("Results (device = %s, numIterations = %d, inputBinding = %s, inputDataType = %s, deviceCreationLocation = %s):\n",
            TypeHelper::Stringify(deviceType).c_str(),
            numIterations,
            TypeHelper::Stringify(inputBindingType).c_str(),
            TypeHelper::Stringify(inputDataType).c_str(),
            TypeHelper::Stringify(deviceCreationLocation).c_str()
        );

        std::cout << "  Load: " << (isnan(loadTime) ? "N/A" : std::to_string(loadTime) + " ms") << std::endl;
        std::cout << "  Bind: " << bindTime << " ms" << std::endl;
        std::cout << "  Evaluate: " << evalTime << " ms" << std::endl;
        std::cout << "  Total Time: " << totalTime << " ms" << std::endl;
        std::cout << "  Working Set Memory usage (evaluate): " << evalMemoryUsage << " MB" << std::endl;
        std::cout << "  Dedicated Memory Usage (evaluate): " << gpuEvalDedicatedMemoryUsage << " MB" << std::endl;
        std::cout << "  Shared Memory Usage (evaluate): " << gpuEvalSharedMemoryUsage << " MB" << std::endl;

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
    
    void SetDefaultCSVFileNamePerIteration()
    {
        auto time = std::time(nullptr);
        struct tm localTime;
        localtime_s(&localTime, &time);

        std::ostringstream oss;
        oss << std::put_time(&localTime, "%Y-%m-%d %H.%M.%S");
        fileNameIter = "PerIterPerf [" + oss.str() + "].csv";
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
        m_csvFileNamePerIteration = converter.from_bytes(fileNameIter);
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

    void WritePerformanceDataToCSV(
        const Profiler<WINML_MODEL_TEST_PERF> &profiler,
        int numIterations, std::wstring model,
        std::string modelBinding,
        std::string inputBinding,
        std::string inputType,
        std::string deviceCreationLocation,
        bool firstRunIgnored
    ) const
    {
        double loadTime = profiler[LOAD_MODEL].GetAverage(CounterType::TIMER);
        double bindTime = profiler[BIND_VALUE].GetAverage(CounterType::TIMER);
        double evalTime = profiler[EVAL_MODEL].GetAverage(CounterType::TIMER);
        double evalMemoryUsage = profiler[EVAL_MODEL].GetAverage(CounterType::WORKING_SET_USAGE);
        double gpuEvalSharedMemoryUsage = profiler[EVAL_MODEL].GetAverage(CounterType::GPU_SHARED_MEM_USAGE);
        double gpuEvalDedicatedMemoryUsage = profiler[EVAL_MODEL].GetAverage(CounterType::GPU_DEDICATED_MEM_USAGE);
        double totalTime = (isnan(loadTime) ? 0 : loadTime) + bindTime + evalTime;

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
                fout << "Model Name" << ","
                     << "Model Binding" << ","
                     << "Input Binding" << ","
                     << "Input Type" << ","
                     << "Device Creation Location" << ","
                     << "Iterations" << ","
                     << "First Run Ignored" << ","
                     << "Load (ms)" << ","
                     << "Bind (ms)" << ","
                     << "Evaluate (ms)" << ","
                     << "Total Time (ms)" << ","
                     << "Working Set Memory usage (evaluate) (MB)" << ","
                     << "GPU Dedicated memory usage (evaluate) (MB)" << ","
                     << "GPU Shared memory usage (evaluate) (MB)" << std::endl;
            }

            fout << modelName << ","
                 << modelBinding << ","
                 << inputBinding << ","
                 << inputType << ","
                 << deviceCreationLocation << ","
                 << numIterations << ","
                 << firstRunIgnored << ","
                 << (isnan(loadTime) ? "N/A" : std::to_string(loadTime)) << ","
                 << bindTime << ","
                 << evalTime << ","
                 << totalTime << ","
                 << evalMemoryUsage << ","
                 << gpuEvalDedicatedMemoryUsage << ","
                 << gpuEvalSharedMemoryUsage << std::endl;

            fout.close();
        }
    }

    void WritePerformanceDataToCSVPerIteration(Profiler<WINML_MODEL_TEST_PERF> &profiler, const CommandLineArgs& args, std::wstring model, std::wstring img)
    {
        if (m_csvFileNamePerIteration.length() > 0)
        {
            bool bNewFile = false;
            std::ifstream fin;
            fin.open(m_csvFileNamePerIteration);
            std::filebuf* outbuf = fin.rdbuf();
            if (EOF == outbuf->sbumpc())
            {
                bNewFile = true;
            }
            fin.close();
            
            std::ofstream fout;
            fout.open(m_csvFileNamePerIteration, std::ios_base::app);
            
            std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
            std::string modelName = converter.to_bytes(model);
            std::string imgName = converter.to_bytes(img);
            
            if (bNewFile)
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
                     << "Evaluate (ms)" << "," << std::endl;
            }

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
                     << m_clockEvalTimes[i] << std::endl;
            }
            fout.close();
        }
    }

    std::vector<double> m_clockLoadTimes;
    std::vector<double> m_clockBindTimes;
    std::vector<double> m_clockEvalTimes;

private:
    std::wstring m_csvFileName;
    std::wstring m_csvFileNamePerIteration;
    std::string fileNameIter;

    std::vector<double> m_EvalTime;
    std::vector<double> m_CPUWorkingDiff;
    std::vector<double> m_CPUWorkingStart;
    std::vector<double> m_GPUSharedDiff;
    std::vector<double> m_GPUSharedStart;
    std::vector<double> m_GPUDedicatedDiff;
};