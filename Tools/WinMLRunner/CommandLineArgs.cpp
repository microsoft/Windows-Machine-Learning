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
    std::cout << "  -perf : capture timing measurements" << std::endl;
    std::cout << "  -iterations : # times perf measurements will be run/averaged" << std::endl;
    std::cout << "  -input <fully qualified path>: binds image or CSV to model" << std::endl;
    std::cout << "  -debug: print trace logs" << std::endl;
}

CommandLineArgs::CommandLineArgs()
{
    int numArgs = 0;
    LPWSTR* args = CommandLineToArgvW(GetCommandLineW(), &numArgs);

    for (int i = 0; i < numArgs; i++)
    {
        if ((_wcsicmp(args[i], L"-CPU") == 0))
        {
            m_useCPU = true;
        }
        else if ((_wcsicmp(args[i], L"-GPU") == 0))
        {
            m_useGPU = true;
            m_deviceKind = LearningModelDeviceKind::DirectX;
        }
        else if ((_wcsicmp(args[i], L"-GPUHighPerformance") == 0))
        {
            m_useGPU = true;
            m_deviceKind = LearningModelDeviceKind::DirectXHighPerformance;
        }
        else if ((_wcsicmp(args[i], L"-GPUMinPower") == 0))
        {
            m_useGPU = true;
            m_deviceKind = LearningModelDeviceKind::DirectXMinPower;
        }
        if ((_wcsicmp(args[i], L"-iterations") == 0) && (i + 1 < numArgs))
        {
            m_numIterations = static_cast<UINT>(_wtoi(args[++i]));
        }
        else if ((_wcsicmp(args[i], L"-model") == 0) && (i + 1 < numArgs))
        {
            m_modelPath = args[++i];
        }
        else if ((_wcsicmp(args[i], L"-folder") == 0) && (i + 1 < numArgs))
        {
            m_modelFolderPath = args[++i];
        }
        else if ((_wcsicmp(args[i], L"-input") == 0))
        {
            m_inputData = args[++i];
        }
        else if ((_wcsicmp(args[i], L"-perf") == 0))
        {
            m_perfCapture = true;
        }
        else if ((_wcsicmp(args[i], L"-debug") == 0))
        {
            m_debug = true;
        }
        else if ((_wcsicmp(args[i], L"/?") == 0))
        {
            PrintUsage();
            return;
        }
    }

    m_useCPUandGPU = m_useCPU == m_useGPU;

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

