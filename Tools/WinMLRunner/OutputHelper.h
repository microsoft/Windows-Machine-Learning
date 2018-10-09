#pragma once
#include "Common.h"
#include <fstream>
#include <ctime>
#include <locale>
#include <utility>
#include <codecvt>
#include <iomanip>

using namespace winrt::Windows::AI::MachineLearning;
using namespace Windows::Storage::Streams;

// Stores performance information and handles output to the command line and CSV files.
class OutputHelper
{
public:
    OutputHelper(bool silent) : m_silent(silent) {}

    void PrintLoadingInfo(const std::wstring& modelPath) const
    {
        if (!m_silent)
        {
            wprintf(L"Loading model (path = %s)...\n", modelPath.c_str());
        }
    }

    void PrintBindingInfo(uint32_t iteration, DeviceType deviceType, InputBindingType inputBindingType, InputDataType inputDataType) const
    {
        if (!m_silent)
        {
            printf(
                "Binding (device = %s, iteration = %d, inputBinding = %s, inputDataType = %s)...",
                TypeHelper::Stringify(deviceType),
                iteration,
                TypeHelper::Stringify(inputBindingType),
                TypeHelper::Stringify(inputDataType)
            );
        }
    }

    void PrintEvaluatingInfo(uint32_t iteration, DeviceType deviceType, InputBindingType inputBindingType, InputDataType inputDataType) const
    {
        if (!m_silent)
        {
            printf(
                "Evaluating (device = %s, iteration = %d, inputBinding = %s, inputDataType = %s)...",
                TypeHelper::Stringify(deviceType),
                iteration,
                TypeHelper::Stringify(inputBindingType),
                TypeHelper::Stringify(inputDataType)
            );
        }
    }

    void PrintModelInfo(std::wstring modelPath, LearningModel model) const
    {
        if (!m_silent)
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
    }

    void PrintFeatureDescriptorInfo(const ILearningModelFeatureDescriptor &descriptor) const
    {
        if (!m_silent)
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
    }

    void PrintHardwareInfo() const
    {
        if (!m_silent)
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
    }

    void PrintResults(const Profiler<WINML_MODEL_TEST_PERF> &profiler, uint32_t numIterations, DeviceType deviceType, InputBindingType inputBindingType, InputDataType inputDataType) const
    {
        double loadTime = profiler[LOAD_MODEL].GetAverage(CounterType::TIMER);
        double bindTime = profiler[BIND_VALUE].GetAverage(CounterType::TIMER);
        double evalTime = profiler[EVAL_MODEL].GetAverage(CounterType::TIMER);
        double evalMemoryUsage = profiler[EVAL_MODEL].GetAverage(CounterType::WORKING_SET_USAGE);
        double gpuEvalSharedMemoryUsage = profiler[EVAL_MODEL].GetAverage(CounterType::GPU_SHARED_MEM_USAGE);
        double gpuEvalDedicatedMemoryUsage = profiler[EVAL_MODEL].GetAverage(CounterType::GPU_DEDICATED_MEM_USAGE);

        double totalBindTime = std::accumulate(m_clockBindTimes.begin(), m_clockBindTimes.end(), 0.0);
        double clockBindTime = totalBindTime / (double)numIterations;

        double totalEvalTime = std::accumulate(m_clockEvalTimes.begin(), m_clockEvalTimes.end(), 0.0);
        double clockEvalTime = totalEvalTime / (double)numIterations;

        if (!m_silent)
        {
            double totalTime = (isnan(loadTime) ? 0 : loadTime) + bindTime + evalTime;

            std::cout << std::endl;

            printf("Results (device = %s, numIterations = %d, inputBinding = %s, inputDataType = %s):\n",
                TypeHelper::Stringify(deviceType),
                numIterations,
                TypeHelper::Stringify(inputBindingType),
                TypeHelper::Stringify(inputDataType)
            );

            printf("  Load: %s\n", isnan(loadTime) ? "N/A" : std::to_string(loadTime) + "ms");
            printf("  Bind: %f ms\n", bindTime);
            printf("  Evaluate: %f ms\n", evalTime);
            printf("  Total Time: %f ms\n", totalTime);
            printf("  Wall-Clock Load: %f ms\n", m_clockLoadTime);
            printf("  Wall-Clock Bind: %f ms\n", clockBindTime);
            printf("  Wall-Clock Evaluate: %f ms\n", clockEvalTime);
            printf("  Total Wall-Clock Time: %f ms\n", m_clockLoadTime + clockBindTime + clockEvalTime);
            printf("  Working Set Memory usage (evaluate): %s\n", isnan(evalMemoryUsage) ? "N/A" : std::to_string(evalMemoryUsage) + " MB");
            printf("  Dedicated Memory Usage (evaluate): %s\n", isnan(gpuEvalDedicatedMemoryUsage) ? "N/A" : std::to_string(gpuEvalDedicatedMemoryUsage) + " MB");
            printf("  Shared Memory Usage (evaluate): %s\n", isnan(gpuEvalSharedMemoryUsage) ? "N/A" : std::to_string(gpuEvalSharedMemoryUsage) + " MB");

            std::cout << std::endl << std::endl;
        }
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

    void WritePerformanceDataToCSV(const Profiler<WINML_MODEL_TEST_PERF> &profiler, int numIterations, std::wstring model, std::string modelBinding, std::string inputBinding, std::string inputType, bool firstRunIgnored) const
    {
        double loadTime = profiler[LOAD_MODEL].GetAverage(CounterType::TIMER);
        double bindTime = profiler[BIND_VALUE].GetAverage(CounterType::TIMER);
        double evalTime = profiler[EVAL_MODEL].GetAverage(CounterType::TIMER);
        double evalMemoryUsage = profiler[EVAL_MODEL].GetAverage(CounterType::WORKING_SET_USAGE);
        double gpuEvalSharedMemoryUsage = profiler[EVAL_MODEL].GetAverage(CounterType::GPU_SHARED_MEM_USAGE);
        double gpuEvalDedicatedMemoryUsage = profiler[EVAL_MODEL].GetAverage(CounterType::GPU_DEDICATED_MEM_USAGE);

        double totalBindTime = std::accumulate(m_clockBindTimes.begin(), m_clockBindTimes.end(), 0.0);
        double clockBindTime = totalBindTime / (double)numIterations;

        double totalEvalTime = std::accumulate(m_clockEvalTimes.begin(), m_clockEvalTimes.end(), 0.0);
        double clockEvalTime = totalEvalTime / (double)numIterations;

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
                     << "Iterations" << ","
                     << "First Run Ignored" << ","
                     << "Load (ms)" << ","
                     << "Bind (ms)" << ","
                     << "Evaluate (ms)" << ","
                     << "Total Time (ms)" << ","
                     << "Working Set Memory usage (evaluate) (MB)" << ","
                     << "GPU Dedicated memory usage (evaluate) (MB)" << ","
                     << "GPU Shared memory usage (evaluate) (MB)" << ","
                     << "Wall-clock Load (ms)" << ","
                     << "Wall-clock Bind (ms)" << ","
                     << "Wall-clock Evaluate (ms)" << ","
                     << "Wall-clock total time (ms)" << std::endl;
            }

            fout << modelName << ","
                 << modelBinding << ","
                 << inputBinding << ","
                 << inputType << ","
                 << numIterations << ","
                 << firstRunIgnored << ","
                 << (isnan(loadTime) ? "N/A" : std::to_string(loadTime)) << ","
                 << bindTime << ","
                 << evalTime << ","
                 << loadTime + bindTime + evalTime << ","
                 << (isnan(evalMemoryUsage) ? "N/A" : std::to_string(evalMemoryUsage)) << ","
                 << (isnan(gpuEvalDedicatedMemoryUsage) ? "N/A" : std::to_string(gpuEvalDedicatedMemoryUsage)) << ","
                 << (isnan(gpuEvalSharedMemoryUsage) ? "N/A" : std::to_string(gpuEvalSharedMemoryUsage)) << ","
                 << m_clockLoadTime << ","
                 << clockBindTime << ","
                 << clockEvalTime << ","
                 << m_clockLoadTime + clockBindTime + clockEvalTime << std::endl;

            fout.close();
        }
    }
    
    void Reset() 
    {
         m_clockEvalTime = 0;
         m_clockLoadTime = 0;
         m_clockBindTime = 0;

         m_clockBindTimes.clear();
         m_clockEvalTimes.clear();
    }

    double m_clockLoadTime = 0;

    std::vector<double> m_clockBindTimes;
    std::vector<double> m_clockEvalTimes;

private:
    std::wstring m_csvFileName;

    double m_clockBindTime = 0;
    double m_clockEvalTime = 0;

    bool m_silent = false;
};