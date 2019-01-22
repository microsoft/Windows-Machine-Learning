#include <Windows.h>
#include <string>
#include <iostream>
#include "CommandLineArgs.h"

using namespace Windows::AI::MachineLearning;

void CommandLineArgs::PrintUsage() {
    std::cout << "WinML Runner" << std::endl;
    std::cout << " ---------------------------------------------------------------" << std::endl;
    std::cout << "WinmlRunner.exe <-model | -folder> <fully qualified path> [options]" << std::endl;
    std::cout << std::endl;
    std::cout << "options: " << std::endl;
    std::cout << "  -CPU : run model on default CPU" << std::endl;
    std::cout << "  -GPU : run model on default GPU" << std::endl;
    std::cout << "  -GPUHighPerformance : run model on GPU with highest performance" << std::endl;
    std::cout << "  -GPUMinPower : run model on GPU with the least power" << std::endl;
    std::cout << "  -CreateDeviceOnClient : create the device on the client and pass it to WinML" << std::endl;
    std::cout << "  -CreateDeviceInWinML : create the device inside WinML" << std::endl;
    std::cout << "  -CPUBoundInput : bind the input to the CPU" << std::endl;
    std::cout << "  -GPUBoundInput : bind the input to the GPU" << std::endl;
    std::cout << "  -RGB : load the input as an RGB image" << std::endl;
    std::cout << "  -BGR : load the input as a BGR image" << std::endl;
    std::cout << "  -tensor : load the input as a tensor" << std::endl;
    std::cout << "  -perf : capture timing measurements" << std::endl;
    std::cout << "  -iterations : # times perf measurements will be run/averaged" << std::endl;
    std::cout << "  -input <fully qualified path>: binds image or CSV to model" << std::endl;
    std::cout << "  -perfOutput optional:<fully qualified path>: csv file to write the perf results to" << std::endl;
    std::cout << "  -IgnoreFirstRun : ignore the first run in the perf results when calculating the average" << std::endl;
    std::cout << "  -savePerIterationPerf : save per iteration performance results to csv file" << std::endl;
    std::cout << "  -debug: print trace logs" << std::endl;
    std::cout << "  -terse: Terse Mode (suppresses repetitive console output)" << std::endl;
    std::cout << "  -autoScale <interpolationMode>: Enable image autoscaling and set the interpolation mode [Nearest, Linear, Cubic, Fant]" << std::endl;
}

CommandLineArgs::CommandLineArgs(std::vector<std::wstring>& args)
{
    for (UINT i = 0; i < args.size(); i++)
    {
        if ((_wcsicmp(args[i].c_str(), L"-CPU") == 0))
        {
            m_useCPU = true;
        }
        else if ((_wcsicmp(args[i].c_str(), L"-GPU") == 0))
        {
            m_useGPU = true;
        }
        else if ((_wcsicmp(args[i].c_str(), L"-GPUHighPerformance") == 0))
        {
            m_useGPUHighPerformance = true;
        }
        else if ((_wcsicmp(args[i].c_str(), L"-GPUMinPower") == 0))
        {
            m_useGPUMinPower = true;
        }
        else if ((_wcsicmp(args[i].c_str(), L"-CreateDeviceOnClient") == 0))
        {
            m_createDeviceOnClient = true;
        }
        else if ((_wcsicmp(args[i].c_str(), L"-CreateDeviceInWinML") == 0))
        {
            m_createDeviceInWinML = true;
        }
        else if ((_wcsicmp(args[i].c_str(), L"-iterations") == 0) && (i + 1 < args.size()))
        {
            m_numIterations = static_cast<UINT>(_wtoi(args[++i].c_str()));
        }
        else if ((_wcsicmp(args[i].c_str(), L"-model") == 0) && (i + 1 < args.size()))
        {
            m_modelPath = args[++i];
        }
        else if ((_wcsicmp(args[i].c_str(), L"-folder") == 0) && (i + 1 < args.size()))
        {
            m_modelFolderPath = args[++i];
        }
        else if ((_wcsicmp(args[i].c_str(), L"-input") == 0))
        {
            m_inputData = args[++i];
        }
        else if ((_wcsicmp(args[i].c_str(), L"-perfOutput") == 0))
        {
            if (i + 1 < args.size() && args[++i][0] != *L"-")
            {
                m_outputPath = args[i];
            }
            m_perfOutput = true;
        }
        else if ((_wcsicmp(args[i].c_str(), L"-RGB") == 0))
        {
            m_useRGB = true;
        }
        else if ((_wcsicmp(args[i].c_str(), L"-BGR") == 0))
        {
            m_useBGR = true;
        }
        else if ((_wcsicmp(args[i].c_str(), L"-tensor") == 0))
        {
            m_useTensor = true;
        }
        else if ((_wcsicmp(args[i].c_str(), L"-CPUBoundInput") == 0))
        {
            m_useCPUBoundInput = true;
        }
        else if ((_wcsicmp(args[i].c_str(), L"-GPUBoundInput") == 0))
        {
            m_useGPUBoundInput = true;
        }
        else if ((_wcsicmp(args[i].c_str(), L"-IgnoreFirstRun") == 0))
        {
            m_ignoreFirstRun = true;
        }
        else if ((_wcsicmp(args[i].c_str(), L"-perf") == 0))
        {
            m_perfCapture = true;
        }
        else if ((_wcsicmp(args[i].c_str(), L"-debug") == 0))
        {
            m_debug = true;
        }
        else if ((_wcsicmp(args[i].c_str(), L"-savePerIterationPerf") == 0))
        {
            m_perIterCapture = true;
        }
        else if ((_wcsicmp(args[i].c_str(), L"-terse") == 0))
        {
            m_terseOutput = true;
        }
        else if ((_wcsicmp(args[i].c_str(), L"-autoScale") == 0) && (i + 1 < args.size()))
        {
            m_autoScale = true;
            if (_wcsicmp(args[++i].c_str(), L"Nearest") == 0)
            {
                m_autoScaleInterpMode = BitmapInterpolationMode::NearestNeighbor;
            }
            else if (_wcsicmp(args[i].c_str(), L"Linear") == 0)
            {
                m_autoScaleInterpMode = BitmapInterpolationMode::Linear;
            }
            else if (_wcsicmp(args[i].c_str(), L"Cubic") == 0)
            {
                m_autoScaleInterpMode = BitmapInterpolationMode::Cubic;
            }
            else if (_wcsicmp(args[i].c_str(), L"Fant") == 0)
            {
                m_autoScaleInterpMode = BitmapInterpolationMode::Fant;
            }
            else
            {
                std::cout << "Unknown AutoScale Interpolation Mode!" << std::endl;
                PrintUsage();
                return;
            }
        }
        else if ((_wcsicmp(args[i].c_str(), L"/?") == 0))
        {
            PrintUsage();
            return;
        }
    }

    if (m_modelPath.empty() && m_modelFolderPath.empty())
    {
        std::cout << std::endl;
        PrintUsage();
        return;
    }

    if (!m_inputData.empty())
    {
        if (m_inputData.find(L".png") != std::string::npos || m_inputData.find(L".jpg") != std::string::npos)
        {
            m_imagePath = m_inputData;
        }
        if (m_inputData.find(L".csv") != std::string::npos)
        {
            m_csvData = m_inputData;
        }
    }
}

