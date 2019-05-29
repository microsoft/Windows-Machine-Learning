#pragma once
#include "Common.h"

class CommandLineArgs
{
public:
    CommandLineArgs(){};
    CommandLineArgs(const std::vector<std::wstring>& args);
    void PrintUsage();
    bool IsConcurrentLoad() const { return m_concurrentLoad; }
    bool IsUsingGPUHighPerformance() const { return m_useGPUHighPerformance; }
    bool IsUsingGPUMinPower() const { return m_useGPUMinPower; }
    bool UseBGR() const { return m_useBGR; }
    bool IsUsingGPUBoundInput() const { return m_useGPUBoundInput; }
    bool IsPerformanceCapture() const { return m_perfCapture; }
    bool IsPerformanceConsoleOutputVerbose() const { return m_perfConsoleOutputAll; }
    bool IsEvaluationDebugOutputEnabled() const { return m_evaluation_debug_output; }
    bool TerseOutput() const { return m_terseOutput; }
    bool IsPerIterationCapture() const { return m_perIterCapture; }
    bool IsCreateDeviceOnClient() const { return m_createDeviceOnClient; }
    bool IsAutoScale() const { return m_autoScale; }
    bool IsOutputPerf() const { return m_perfOutput; }
    bool IsSaveTensor() const { return m_saveTensor; }

    BitmapInterpolationMode AutoScaleInterpMode() const { return m_autoScaleInterpMode; }

    const std::vector<std::wstring>& ImagePaths() const { return m_imagePaths; }
    const std::wstring& CsvPath() const { return m_csvData; }
    const std::wstring& OutputPath() const { return m_perfOutputPath; }
    const std::wstring& FolderPath() const { return m_modelFolderPath; }
    const std::wstring& ModelPath() const { return m_modelPath; }
    const std::wstring& PerIterationDataPath() const { return m_perIterationDataPath; }
    std::vector<std::pair<std::string, std::string>>& GetPerformanceFileMetadata() { return m_perfFileMetadata; }
#ifdef DXCORE_SUPPORTED_BUILD
    const std::wstring& GetGPUAdapterName() const { return m_adapterName; }
#endif

    bool UseRGB() const
    {
        // If an image is specified without flags, we load it as a BGR image by default
        return m_useRGB || (!m_imagePaths.empty() && !m_useBGR && !m_useTensor);
    }

    bool UseTensor() const
    {
        // Tensor input is the default input if no flag is specified
        return m_useTensor || (!m_useBGR && !UseRGB());
    }

    bool UseGPU() const { return m_useGPU || (!m_useCPU && !m_useGPUHighPerformance && !m_useGPUMinPower); }

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

    bool CreateDeviceInWinML() const
    {
        // By Default we create the device in WinML if no flag is specified
        return m_createDeviceInWinML || !m_createDeviceOnClient;
    }

    bool IsGarbageInput() const
    {
        // When there is no image or csv input provided, then garbage input binding is used.
        return m_imagePaths.empty() && m_csvData.empty();
    }
    bool IsCSVInput() const { return m_imagePaths.empty() && !m_csvData.empty(); }
    bool IsImageInput() const { return !m_imagePaths.empty() && m_csvData.empty(); }

    uint32_t NumIterations() const { return m_numIterations; }
    uint32_t NumThreads() const { return m_numThreads; }
    uint32_t ThreadInterval() const { return m_threadInterval; } // Thread interval in milliseconds
    uint32_t TopK() const { return m_topK; }

    void ToggleCPU(bool useCPU) { m_useCPU = useCPU; }
    void ToggleGPU(bool useGPU) { m_useGPU = useGPU; }
    void ToggleGPUHighPerformance(bool useGPUHighPerformance) { m_useGPUHighPerformance = useGPUHighPerformance; }
    void ToggleUseGPUMinPower(bool useGPUMinPower) { m_useGPUMinPower = useGPUMinPower; }
    void ToggleConcurrentLoad(bool concurrentLoad) { m_concurrentLoad = concurrentLoad; }
    void ToggleCreateDeviceOnClient(bool createDeviceOnClient) { m_createDeviceOnClient = createDeviceOnClient; }
    void ToggleCreateDeviceInWinML(bool createDeviceInWinML) { m_createDeviceInWinML = createDeviceInWinML; }
    void ToggleCPUBoundInput(bool useCPUBoundInput) { m_useCPUBoundInput = useCPUBoundInput; }
    void ToggleGPUBoundInput(bool useGPUBoundInput) { m_useGPUBoundInput = useGPUBoundInput; }
    void ToggleUseRGB(bool useRGBImage) { m_useRGB = useRGBImage; }
    void ToggleUseBGR(bool useBGRImage) { m_useBGR = useBGRImage; }
    void ToggleUseTensor(bool useTensor) { m_useTensor = useTensor; }
    void TogglePerformanceCapture(bool perfCapture) { m_perfCapture = perfCapture; }
    void ToggleIgnoreFirstRun(bool ignoreFirstRun) { m_ignoreFirstRun = ignoreFirstRun; }
    void TogglePerIterationPerformanceCapture(bool perIterCapture) { m_perIterCapture = perIterCapture; }
    void ToggleEvaluationDebugOutput(bool debug) { m_evaluation_debug_output = debug; }
    void ToggleTerseOutput(bool terseOutput) { m_terseOutput = terseOutput; }
    void TogglePerfOutput(bool perfOutput) { m_perfOutput = perfOutput; }

    void SetModelPath(const std::wstring& modelPath) { m_modelPath = modelPath; }
    void SetPerIterationDataPath(const std::wstring& perIterationDataPath)
    {
        m_perIterationDataPath = perIterationDataPath;
    }
    void SetInputDataPath(const std::wstring& inputDataPath) { m_inputData = inputDataPath; }
    void SetNumThreads(unsigned numThreads) { m_numThreads = numThreads; }
    void SetThreadInterval(unsigned threadInterval) { m_threadInterval = threadInterval; }
    void SetTopK(unsigned k) { m_topK = k; }
    void SetPerformanceCSVPath(const std::wstring& performanceCSVPath) { m_perfOutputPath = performanceCSVPath; }
    void SetRunIterations(const uint32_t iterations) { m_numIterations = iterations; }
    void AddPerformanceFileMetadata(const std::string& key, const std::string& value)
    {
        m_perfFileMetadata.push_back(std::make_pair(key, value));
    }
    std::wstring SaveTensorMode() const { return m_saveTensorMode; }

private:
    bool m_perfCapture = false;
    bool m_perfConsoleOutputAll = false;
    bool m_useCPU = false;
    bool m_useGPU = false;
    bool m_useGPUHighPerformance = false;
    bool m_useGPUMinPower = false;
    bool m_concurrentLoad = false;
    bool m_createDeviceOnClient = false;
    bool m_createDeviceInWinML = false;
    bool m_useRGB = false;
    bool m_useBGR = false;
    bool m_useTensor = false;
    bool m_useCPUBoundInput = false;
    bool m_useGPUBoundInput = false;
    bool m_ignoreFirstRun = false;
    bool m_evaluation_debug_output = false;
    bool m_perIterCapture = false;
    bool m_terseOutput = false;
    bool m_autoScale = false;
    bool m_perfOutput = false;
    BitmapInterpolationMode m_autoScaleInterpMode = BitmapInterpolationMode::Cubic;
    bool m_saveTensor = false;
    std::wstring m_saveTensorMode = L"First";

    std::wstring m_modelFolderPath;
    std::wstring m_modelPath;
    std::vector<std::wstring> m_imagePaths;
    std::wstring m_inputImageFolderPath;
    std::wstring m_csvData;
    std::wstring m_inputData;
#ifdef DXCORE_SUPPORTED_BUILD
    std::wstring m_adapterName;
#endif
    std::wstring m_perfOutputPath;
    std::wstring m_perIterationDataPath;
    uint32_t m_numIterations = 1;
    uint32_t m_numThreads = 1;
    uint32_t m_threadInterval = 0;
    uint32_t m_topK = 1;
    std::vector<std::pair<std::string, std::string>> m_perfFileMetadata;

    void CheckNextArgument(const std::vector<std::wstring>& args, UINT i);
    void CheckForInvalidArguments();
    void SetupOutputDirectories(const std::wstring& sBaseOutputPath, const std::wstring& sPerfOutputPath,
                                const std::wstring& sPerIterationDataPath);
    void PopulateInputImagePaths();
};
