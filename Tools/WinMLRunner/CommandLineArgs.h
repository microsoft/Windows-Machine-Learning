#pragma once
#include "Common.h"

class CommandLineArgs
{
public:
    CommandLineArgs();
    void PrintUsage();

    bool UseGPU() const { return m_useGPU; }
    bool UseGPUHighPerformance() const { return m_useGPUHighPerformance; }
    bool UseGPUMinPower() const { return m_useGPUMinPower; }
    bool UseRGB() const { return m_useRGB; }
    bool UseGPUBoundInput() const { return m_useGPUBoundInput; }
    bool IgnoreFirstRun() const { return m_ignoreFirstRun; }
    bool PerfCapture() const { return m_perfCapture; }
    bool EnableDebugOutput() const { return m_debug; }
    bool Silent() const { return m_silent; }
   
    const std::wstring& ImagePath() const { return m_imagePath; }
    const std::wstring& CsvPath() const { return m_csvData; }
    const std::wstring& OutputPath() const { return m_outputPath; }
    const std::wstring& FolderPath() const { return m_modelFolderPath; }
    const std::wstring& ModelPath() const { return m_modelPath; }

    void SetModelPath(std::wstring path) { m_modelPath = path; }

    bool UseBGR() const
    {
        // If an image is specified without flags, we load it as a BGR image by default
        return m_useRGB || (!m_imagePath.empty() && !m_useRGB && !m_useTensor);
    }

    bool UseTensor() const
    {
        // Tensor input is the default input if no flag is specified
        return m_useTensor || (!m_useRGB && !UseBGR());
    }

    bool UseCPU() const
    {
        // CPU is the default device if no flag is specified
        return m_useCPU || (!m_useGPU && !m_useGPUHighPerformance && !m_useGPUMinPower);
    }

    bool UseCPUBoundInput() const
    {
        // CPU is the default input binding if no flag is specified
        return m_useCPUBoundInput || !m_useGPUBoundInput;
    }

    uint32_t NumIterations() const { return m_numIterations; }

private:
    bool m_perfCapture = false;
    bool m_useCPU = false;
    bool m_useGPU = false;
    bool m_useGPUHighPerformance = false;
    bool m_useGPUMinPower = false;
    bool m_useRGB = false;
    bool m_useBGR = false;
    bool m_useTensor = false;
    bool m_useCPUBoundInput = false;
    bool m_useGPUBoundInput = false;
    bool m_ignoreFirstRun = false;
    bool m_debug = false;
    bool m_silent = false;

    std::wstring m_modelFolderPath;
    std::wstring m_modelPath;
    std::wstring m_imagePath;
    std::wstring m_csvData;
    std::wstring m_inputData;
    std::wstring m_outputPath;
    uint32_t m_numIterations = 1;
};