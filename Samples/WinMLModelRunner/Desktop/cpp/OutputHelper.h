#pragma once
#include "Common.h"
#include <time.h>
#include "CommandLineArgs.h"
#include <fstream>
#include <ctime>
#include <locale>
#include <utility>
#include <codecvt>
#include <iomanip>
#include <windows.h>
#include <stdio.h>

using namespace winrt::Windows::AI::MachineLearning;

// Stores performance information and handles output to the command line and CSV files.
class OutputHelper
{
public:
    OutputHelper() {}

    void PrintWallClockTimes(UINT iterations)
    {
        double totalEvalTime = std::accumulate(m_clockEvalTimes.begin(), m_clockEvalTimes.end(), 0.0);
        m_clockEvalTime = totalEvalTime / (double)iterations;

        std::cout << "Wall-clock Time Averages (iterations = " << iterations << "):" << std::endl;
        std::cout << "	Load: " << m_clockLoadTime << " ms" << std::endl;
        std::cout << "	Bind: " << m_clockBindTime << " ms" << std::endl;
        std::cout << "	Evaluate: " << m_clockEvalTime << " ms" << std::endl;
        std::cout << "	Total time: " << m_clockLoadTime + m_clockBindTime + m_clockEvalTime << " ms" << std::endl;
        std::cout << std::endl;
    }

    void PrintCPUTimes(Profiler<WINML_MODEL_TEST_PERF> &profiler, UINT iterations)
    {
         m_CPULoadTime = profiler[LOAD_MODEL].GetAverage(CounterType::TIMER);
         m_CPUBindTime = profiler[BIND_VALUE].GetAverage(CounterType::TIMER);
         m_CPUEvalTime = profiler[EVAL_MODEL].GetAverage(CounterType::TIMER);
         m_CPUEvalMemoryUsage = profiler[EVAL_MODEL].GetAverage(CounterType::CPU_USAGE);

        std::cout << "CPU Time Averages (iterations = " << iterations << "):" << std::endl;
        std::cout << "	Load: " << m_CPULoadTime << " ms" << std::endl;
        std::cout << "	Bind: " << m_CPUBindTime << " ms" << std::endl;
        std::cout << "	Evaluate: " << m_CPUEvalTime << " ms" << std::endl;
        std::cout << "	Total time: " << m_CPULoadTime + m_CPUBindTime + m_CPUEvalTime << " ms" << std::endl;
        std::cout << "	Evaluate memory usage: " << m_CPUEvalMemoryUsage << " mb" << std::endl;
        std::cout << std::endl;
    }

    void PrintGPUTimes(Profiler<WINML_MODEL_TEST_PERF> &profiler, UINT iterations)
    {
         m_GPULoadTime = profiler[LOAD_MODEL].GetAverage(CounterType::TIMER);
         m_GPUBindTime = profiler[BIND_VALUE].GetAverage(CounterType::TIMER);
         m_GPUEvalTime = profiler[EVAL_MODEL].GetAverage(CounterType::TIMER);
         m_GPUEvalMemoryUsage = profiler[EVAL_MODEL].GetAverage(CounterType::CPU_USAGE);

        std::cout << "GPU Time Averages (iterations = " << iterations << "):" << std::endl;
        std::cout << "	Load: " << m_GPULoadTime << " ms" << std::endl;
        std::cout << "	Bind: " << m_GPUBindTime << " ms" << std::endl;
        std::cout << "	Evaluate: " << m_GPUEvalTime << " ms" << std::endl;
        std::cout << "	Total time: " << m_GPULoadTime + m_GPUBindTime + m_GPUEvalTime << " ms" << std::endl;
        std::cout << "	Evaluate memory usage: " << m_GPUEvalMemoryUsage << " mb" << std::endl;
        std::cout << std::endl;
    }

    void PrintModelInfo(std::wstring modelName, LearningModelDeviceKind deviceKind)
    {
        std::wstring device = deviceKind == LearningModelDeviceKind::Cpu ? L" [CPU]" : L" [GPU]";
        std::wcout << modelName << device << std::endl;
        std::cout << "=================================================================" << std::endl;
        std::cout << std::endl;
    }

    void PrintHardwareInfo()
    {
        std::cout << "WinML Model Runner" << std::endl;

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
        std::string fileName = "WinML Model Run [" + oss.str() + "].csv";
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
        m_csvFileName = converter.from_bytes(fileName);
    }

    void WritePerformanceDataToCSV(Profiler<WINML_MODEL_TEST_PERF> &g_Profiler, CommandLineArgs args, std::wstring model)
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
                        << "CPU Usage (Evaluate) (mb)" << ",";
                }
                if (args.UseCPUandGPU() || args.UseGPU())
                {

                    fout << "GPU Load (ms)" << ","
                        << "GPU Bind (ms)" << ","
                        << "GPU Evaluate (ms)" << ","
                        << "GPU total time (ms)" << ","
                        << "GPU Usage (Evaluate) (mb)" << ",";
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
                fout << m_GPULoadTime << ","
                << m_GPUBindTime << ","
                << m_GPUEvalTime << ","
                << m_GPULoadTime + m_GPUBindTime + m_GPUEvalTime << ","
                << m_GPUEvalMemoryUsage << ",";
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

         m_GPULoadTime = 0;
         m_GPUBindTime = 0;
         m_GPUEvalTime = 0;
         m_GPUEvalMemoryUsage = 0;

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
    double m_GPULoadTime = 0;
    double m_GPUBindTime = 0;
    double m_GPUEvalTime = 0;
    double m_GPUEvalMemoryUsage = 0;

    double m_CPULoadTime = 0;
    double m_CPUBindTime = 0;
    double m_CPUEvalTime = 0;
    double m_CPUEvalMemoryUsage = 0;

    double m_clockEvalTime = 0;

};