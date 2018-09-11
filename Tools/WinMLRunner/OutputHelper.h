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
using namespace Windows::Foundation::Collections;
using namespace Windows::Storage;
using namespace Windows::Storage::Streams;
using namespace Windows::Media;
using namespace Windows::Graphics::Imaging;

// Stores performance information and handles output to the command line and CSV files.
class OutputHelper
{
public:
    OutputHelper() {}

    void PrintWallClockTimes(UINT iterations)
    {
        double totalEvalTime = std::accumulate(m_clockEvalTimes.begin(), m_clockEvalTimes.end(), 0.0);
        m_clockEvalTime = totalEvalTime / (double)iterations;

        std::cout << std::endl;
        std::cout << "Wall-clock Time Averages (iterations = " << iterations << "):" << std::endl;
        std::cout << "  Load: " << m_clockLoadTime << " ms" << std::endl;
        std::cout << "  Bind: " << m_clockBindTime << " ms" << std::endl;
        std::cout << "  Evaluate: " << m_clockEvalTime << " ms" << std::endl;
        std::cout << "  Total time: " << m_clockLoadTime + m_clockBindTime + m_clockEvalTime << " ms" << std::endl;
        std::cout << std::endl;
    }

    void PrintCPUTimes(Profiler<WINML_MODEL_TEST_PERF> &profiler, UINT iterations)
    {
         m_CPULoadTime = profiler[LOAD_MODEL].GetAverage(CounterType::TIMER);
         m_CPUBindTime = profiler[BIND_VALUE].GetAverage(CounterType::TIMER);
         m_CPUEvalTime = profiler[EVAL_MODEL].GetAverage(CounterType::TIMER);
         m_CPUEvalMemoryUsage = profiler[EVAL_MODEL].GetAverage(CounterType::WORKING_SET_USAGE);

        std::cout << std::endl;
        std::cout << "CPU Time Averages (iterations = " << iterations << "):" << std::endl;
        std::cout << "  Load: " << m_CPULoadTime << " ms" << std::endl;
        std::cout << "  Bind: " << m_CPUBindTime << " ms" << std::endl;
        std::cout << "  Evaluate: " << m_CPUEvalTime << " ms" << std::endl;
        std::cout << "  Total time: " << m_CPULoadTime + m_CPUBindTime + m_CPUEvalTime << " ms" << std::endl;
        std::cout << "  Working Set Memory usage (evaluate): " << m_CPUEvalMemoryUsage << " MB" << std::endl;
        std::cout << std::endl;
    }

    void PrintGPUTimes(Profiler<WINML_MODEL_TEST_PERF> &profiler, UINT iterations)
    {
         m_GPUBindTime = profiler[BIND_VALUE].GetAverage(CounterType::TIMER);
         m_GPUEvalTime = profiler[EVAL_MODEL].GetAverage(CounterType::TIMER);
         m_GPUEvalSharedMemoryUsage = profiler[EVAL_MODEL].GetAverage(CounterType::GPU_SHARED_MEM_USAGE);
         m_GPUEvalDedicatedMemoryUsage = profiler[EVAL_MODEL].GetAverage(CounterType::GPU_DEDICATED_MEM_USAGE);

        std::cout << std::endl;
        std::cout << "GPU Time Averages (iterations = " << iterations << "):" << std::endl;
        std::cout << "  Load: " << "N/A" << std::endl;
        std::cout << "  Bind: " << m_GPUBindTime << " ms" << std::endl;
        std::cout << "  Evaluate: " << m_GPUEvalTime << " ms" << std::endl;
        std::cout << "  Total time: " << m_GPUBindTime + m_GPUEvalTime << " ms" << std::endl;
        std::cout << "  Dedicated memory usage (evaluate): " << m_GPUEvalDedicatedMemoryUsage << " MB" << std::endl;
        std::cout << "  Shared memory usage (evaluate): " << m_GPUEvalSharedMemoryUsage << " MB" << std::endl;
        std::cout << std::endl;
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

    static void PrintFeatureDescriptorInfo(const winrt::Windows::AI::MachineLearning::ILearningModelFeatureDescriptor &descriptor)
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
        std::wcout <<"Feature Kind: " << FeatureDescriptorToString(descriptor)<< std::endl;
        std::cout << std::endl;
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

    void PrintModelInfo(std::wstring modelPath, LearningModel model)
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

    void PrintHardwareInfo()
    {
        std::cout << "WinML Runner" << std::endl;

        com_ptr<IDXGIFactory6> factory;
        (CreateDXGIFactory1(__uuidof(IDXGIFactory6), factory.put_void()));
        com_ptr<IDXGIAdapter> adapter;
        factory->EnumAdapters(0, adapter.put());
        DXGI_ADAPTER_DESC description;
        if (SUCCEEDED(adapter->GetDesc(&description)))
        {
            std::wcout << L"GPU: " << description.Description << std::endl;
            std::cout << std::endl;
        }
    }

    void SetDefaultCSVFileName() 
    {
        auto time = std::time(nullptr);
        struct tm localTime;
        localtime_s(&localTime, &time);

        std::ostringstream oss;
        oss << std::put_time(&localTime, "%Y-%m-%d");
        std::string fileName = "WinML Runner [" + oss.str() + "].csv";
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
        m_csvFileName = converter.from_bytes(fileName);
    }

    void WritePerformanceDataToCSV(Profiler<WINML_MODEL_TEST_PERF> &g_Profiler, const CommandLineArgs& args, std::wstring model)
    {
        if (m_csvFileName.length() > 0)
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
                     << "Iterations" << ",";

                if (args.UseCPUandGPU() || args.UseCPU()) 
                {
                    fout << "CPU Load (ms)" << ","
                        << "CPU Bind (ms)" << ","
                        << "CPU Evaluate (ms)" << ","
                        << "CPU total time (ms)" << ","
                        << "Working Set Memory usage (Evaluate) (MB)" << ",";
                }
                if (args.UseCPUandGPU() || args.UseGPU())
                {

                    fout << "GPU Load (ms)" << ","
                        << "GPU Bind (ms)" << ","
                        << "GPU Evaluate (ms)" << ","
                        << "GPU total time (ms)" << ","
                        << "Dedicated memory usage (evaluate) (MB)" << ","
                        << "Shared memory usage (evaluate) (MB)" << ",";
                }

                    fout << "Wall-clock Load (ms)" << ","
                         << "Wall-clock Bind (ms)" << ","
                         << "Wall-clock Evaluate (ms)" << ","
                         << "Wall-clock total time (ms)" << ","
                         << std::endl;
            }

            fout << modelName << "," << args.NumIterations() << ",";

            if (args.UseCPUandGPU() || args.UseCPU())
            {
                fout << m_CPULoadTime << ","
                << m_CPUBindTime << ","
                << m_CPUEvalTime << ","
                << m_CPULoadTime + m_CPUBindTime + m_CPUEvalTime << ","
                << m_CPUEvalMemoryUsage << ",";
            }
            if (args.UseCPUandGPU() || args.UseGPU())
            {
                fout << "N/A" << ","
                << m_GPUBindTime << ","
                << m_GPUEvalTime << ","
                << m_GPUBindTime + m_GPUEvalTime << ","
                << m_GPUEvalDedicatedMemoryUsage << ","
                << m_GPUEvalSharedMemoryUsage;
            }

            fout << m_clockLoadTime << ","
            << m_clockBindTime << ","
            << m_clockEvalTime << ","
            << m_clockLoadTime + m_clockBindTime + m_clockEvalTime << ","
            << std::endl;
            fout.close();
        }
    }
    
    void Reset() 
    {
         m_GPUBindTime = 0;
         m_GPUEvalTime = 0;
         m_GPUEvalSharedMemoryUsage = 0;
         m_GPUEvalDedicatedMemoryUsage = 0;

         m_CPULoadTime = 0;
         m_CPUBindTime = 0;
         m_CPUEvalTime = 0;
         m_CPUEvalMemoryUsage = 0;


         m_clockLoadTime = 0;
         m_clockBindTime = 0;
         m_clockEvalTime = 0;
    }

    double m_clockLoadTime = 0;
    double m_clockBindTime = 0;
    std::vector<double> m_clockEvalTimes;

    std::wstring m_csvFileName;

private:
    double m_GPUBindTime = 0;
    double m_GPUEvalTime = 0;
    double m_GPUEvalSharedMemoryUsage = 0;
    double m_GPUEvalDedicatedMemoryUsage = 0;

    double m_CPULoadTime = 0;
    double m_CPUBindTime = 0;
    double m_CPUEvalTime = 0;
    double m_CPUEvalMemoryUsage = 0;

    double m_clockEvalTime = 0;

};