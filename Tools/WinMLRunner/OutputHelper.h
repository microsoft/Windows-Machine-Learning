#pragma once
#include "Common.h"
#include "CommandLineArgs.h"
#include <fstream>
#include <ctime>
#include <locale>
#include <utility>
#include <codecvt>
#include <iomanip>
#include <direct.h>

using namespace winrt::Windows::AI::MachineLearning;
using namespace winrt::Windows::Storage::Streams;

// Stores performance information and handles output to the command line and CSV files.
class OutputHelper
{
public:
    OutputHelper(int numIterations, bool silent)
    {
        m_silent = silent;
        m_outputResult.resize(numIterations, "");
        m_outputTensorHash.resize(numIterations, 0);
    }

    void PrintLoadingInfo(const std::wstring& modelPath) const
    {
        if (!m_silent)
        {
            wprintf(L"Loading model (path = %s)...\n", modelPath.c_str());
        }
    }

    void PrintBindingInfo(uint32_t iteration, DeviceType deviceType, InputBindingType inputBindingType, InputDataType inputDataType, DeviceCreationLocation deviceCreationLocation) const
    {
        if (!m_silent)
        {
            printf(
                "Binding (device = %s, iteration = %d, inputBinding = %s, inputDataType = %s, deviceCreationLocation = %s)...",
                TypeHelper::Stringify(deviceType).c_str(),
                iteration,
                TypeHelper::Stringify(inputBindingType).c_str(),
                TypeHelper::Stringify(inputDataType).c_str(),
                TypeHelper::Stringify(deviceCreationLocation).c_str()
            );
        }
    }

    void PrintEvaluatingInfo(uint32_t iteration, DeviceType deviceType, InputBindingType inputBindingType, InputDataType inputDataType, DeviceCreationLocation deviceCreationLocation) const
    {
        if (!m_silent)
        {
            printf(
                "Evaluating (device = %s, iteration = %d, inputBinding = %s, inputDataType = %s, deviceCreationLocation = %s)...",
                TypeHelper::Stringify(deviceType).c_str(),
                iteration,
                TypeHelper::Stringify(inputBindingType).c_str(),
                TypeHelper::Stringify(inputDataType).c_str(),
                TypeHelper::Stringify(deviceCreationLocation).c_str()
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

        if (!m_silent)
        {
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

    void SaveResult(uint32_t iterationNum, std::string result, int hashcode)
    {
        m_outputResult[iterationNum] = result;
        m_outputTensorHash[iterationNum] = hashcode;
    }
    
    void SetDefaultFolder()
    {
        auto time = std::time(nullptr);
        struct tm localTime;
        localtime_s(&localTime, &time);
        std::string cur_dir = _getcwd(NULL, 0);
        std::ostringstream oss;
        oss << std::put_time(&localTime, "%Y-%m-%d_%H.%M.%S");
        std::string folderName = "\\Run[" + oss.str() + "]";
        m_folderNamePerIteration = cur_dir + folderName;
        if (_mkdir(m_folderNamePerIteration.c_str()) != 0)
            std::cout << "Folder cannot be created";
    }

    void SetDefaultCSVResult()
    {
        std::string fileNameResult = m_folderNamePerIteration + "\\Result[OutputTensor]Main.csv";
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
        m_csvFileNameResult = converter.from_bytes(fileNameResult);
    }

    void SetDefaultCSVIterationResult(uint32_t iterationNum, const CommandLineArgs &args)
    {
        if (args.UseCPU() && args.UseGPU())
        {
            if (!m_flagGpuDevice)
            {
                m_fileNameResultDevice = m_folderNamePerIteration + "\\Result[OutputTensor]IterationCpu";
                if (iterationNum == args.NumIterations() || args.SaveTensorMode() == "First")
                {
                    m_flagGpuDevice = true;
                }
            }
            else
            {
                m_fileNameResultDevice = m_folderNamePerIteration + "\\Result[OutputTensor]IterationGpu";
            }
        }
        else if (args.UseGPU())
        {
            m_fileNameResultDevice = m_folderNamePerIteration + "\\Result[OutputTensor]IterationGpu";
        }
        else
        {
            m_fileNameResultDevice = m_folderNamePerIteration + "\\Result[OutputTensor]IterationCpu";
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

    void WriteResultToCSV(const CommandLineArgs &args)
    {
        if (!m_csvFileNameResult.empty())
        {
            bool bNewFile = false;
            std::ifstream fin;
            fin.open(m_csvFileNameResult);
            std::filebuf* outbuf = fin.rdbuf();
            if (EOF == outbuf->sbumpc())
            {
                bNewFile = true;
            }
            fin.close();

            std::ofstream fout;
            fout.open(m_csvFileNameResult, std::ios_base::app);
            
            if (bNewFile)
            {
                fout << "Iteration" << "," << "Result" << "," << "OutputTensorHash" << "," << "FileName" << std::endl;
            }

            
            for (uint32_t i = 0; i < args.NumIterations(); i++)
            {
                fout << i + 1 << "," << m_outputResult[i] << "," << m_outputTensorHash[i] << "," << m_fileNameResultDevice + std::to_string(i + 1) + ".csv" << std::endl;
                if (args.SaveTensorMode() == "First" && i == 0)
                {
                    break;
                }
            }
            fout.close();
        }
    }

    template<typename T>
    void WriteTensorResultToCSV(winrt::Windows::Foundation::Collections::IVectorView<T> &m_Res, uint32_t iterationNum, const CommandLineArgs &args)
    {
        if (args.SaveTensorMode() == "First" && iterationNum > 1)
        {
            return;
        }
        SetDefaultCSVIterationResult(iterationNum, args);
        if (m_csvFileNamePerIterationResult.length() > 0)
        {
            std::ofstream fout;
            fout.open(m_csvFileNamePerIterationResult, std::ios_base::app);
            fout << "Index" << "," << "Value" << std::endl;
            for (int i = 0; i < m_Res.Size(); i++)
            {
                fout << i << "," << m_Res.GetAt(i) << std::endl;
            }
            fout.close();
        }
    }

    template<>
    void WriteTensorResultToCSV(winrt::Windows::Foundation::Collections::IVectorView<winrt::hstring> &m_Res, uint32_t iterationNum, const CommandLineArgs &args)
    {
        if (args.SaveTensorMode() == "First" && iterationNum > 1)
        {
            return;
        }
        SetDefaultCSVIterationResult(iterationNum, args);
        if (m_csvFileNamePerIterationResult.length() > 0)
        {
            std::ofstream fout;
            fout.open(m_csvFileNamePerIterationResult, std::ios_base::app);
            fout << "Result" << std::endl << m_Res.GetAt(0).data() << std::endl;
            fout.close();
        }
    }

    void WriteSequenceResultToCSV(winrt::Windows::Foundation::Collections::IMap<int64_t, float> &m_Map, uint32_t iterationNum, const CommandLineArgs &args)
    {
        SetDefaultCSVIterationResult(iterationNum, args);
        if (m_csvFileNamePerIterationResult.length() > 0)
        {   
            std::ofstream fout;
            fout.open(m_csvFileNamePerIterationResult, std::ios_base::app);
            auto iter = m_Map.First(); 
            fout << "Key" << "," << "Value" << std::endl;
            iter = m_Map.First();
            while (iter.HasCurrent())
            {
                auto pair = iter.Current();
                fout << pair.Key() << "," << pair.Value() << "," << std::endl;
                iter.MoveNext();
            }
            fout.close();
        }
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

private:
    std::wstring m_csvFileName;
    std::wstring m_csvFileNameResult;
    std::wstring m_csvFileNamePerIterationResult;
    std::string m_folderNamePerIteration;
    std::string m_fileNameResultDevice;

    double m_clockBindTime = 0;
    double m_clockEvalTime = 0;

    bool m_silent = false;
    bool m_flagGpuDevice = false;

    std::vector<std::string> m_outputResult;
    std::vector<int> m_outputTensorHash;
};