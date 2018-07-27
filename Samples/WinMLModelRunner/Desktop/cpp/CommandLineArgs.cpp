#include <Windows.h>
#include <string>
#include "CommandLineArgs.h"

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
            m_folderPath = args[++i];
        }
        else if ((_wcsicmp(args[i], L"-disableMetacommands") == 0))
        {
            m_metacommandsEnabled = false;
        }
        else if ((_wcsicmp(args[i], L"-csv") == 0))
        {
            m_csvFileName = args[++i];
        }
    }
    m_useCPUandGPU = m_useCPU == m_useGPU;
}