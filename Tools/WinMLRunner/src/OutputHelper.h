#if defined(_AMD64_)
// PIX markers only work on amd64
#include <DXProgrammableCapture.h>
#include "TimerHelper.h"
#include "LearningModelDeviceHelper.h"
#endif

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
        m_outputResult.resize(numIterations, "");
        m_outputTensorHash.resize(numIterations, 0);
    }

    void PrintLoadingInfo(const std::wstring& modelPath) const;
    void PrintBindingInfo(uint32_t iteration, DeviceType deviceType, InputBindingType inputBindingType,
                          InputDataType inputDataType, DeviceCreationLocation deviceCreationLocation,
                          const std::string& status) const;
    void PrintEvaluatingInfo(uint32_t iteration, DeviceType deviceType, InputBindingType inputBindingType,
                             InputDataType inputDataType, DeviceCreationLocation deviceCreationLocation,
                             const std::string& status) const;
    void PrintModelInfo(std::wstring modelPath, LearningModel model) const;
    void PrintFeatureDescriptorInfo(const ILearningModelFeatureDescriptor& descriptor) const;
    void PrintHardwareInfo() const;
    void PrintResults(const Profiler<WINML_MODEL_TEST_PERF>& profiler, uint32_t numIterations, DeviceType deviceType,
                      InputBindingType inputBindingType, InputDataType inputDataType,
                      DeviceCreationLocation deviceCreationLocation, bool isPerformanceConsoleOutputVerbose) const;
    void SaveLoadTimes(Profiler<WINML_MODEL_TEST_PERF>& profiler, uint32_t iterNum);
    void SaveBindTimes(Profiler<WINML_MODEL_TEST_PERF>& profiler, uint32_t iterNum);
    void SaveEvalPerformance(Profiler<WINML_MODEL_TEST_PERF>& profiler, uint32_t iterNum);
    void SaveResult(uint32_t iterationNum, std::string result, int hashcode);
    void SetDefaultPerIterationFolder(const std::wstring& folderName);
    void SetDefaultCSVFileNamePerIteration();
    std::wstring GetDefaultCSVFileNamePerIteration();
    std::wstring GetCsvFileNamePerIterationResult();
    void SetDefaultCSVIterationResult(uint32_t iterationNum, const CommandLineArgs& args, std::wstring& featureName);
    void SetCSVFileName(const std::wstring& fileName);
    void WritePerIterationPerformance(const CommandLineArgs& args, const std::wstring model,
                                      const std::wstring imagePath);
    void WritePerformanceDataToCSV(const Profiler<WINML_MODEL_TEST_PERF>& profiler, int numIterations,
                                   std::wstring model, const std::string& deviceType, const std::string& inputBinding,
                                   const std::string& inputType, const std::string& deviceCreationLocation,
                                   const std::vector<std::pair<std::string, std::string>>& perfFileMetadata) const;
    static void PrintLearningModelDevice(const LearningModelDeviceWithMetadata& device);
    static std::wstring FeatureDescriptorToString(const ILearningModelFeatureDescriptor& descriptor);
    static bool doesDescriptorContainFP16(const ILearningModelFeatureDescriptor& descriptor);
    static bool doesModelContainFP16(const LearningModel model);
    template <typename T>
    static void ProcessTensorResult(const CommandLineArgs& args, const void* buffer, const uint32_t uCapacity,
                                    std::vector<std::pair<float, int>>& maxValues, std::ofstream& fout, unsigned int k);
    // PIX markers only work on amd64
    com_ptr<IDXGraphicsAnalysis>& GetGraphicsAnalysis() { return m_graphicsAnalysis; }

private:
    std::vector<double> m_clockLoadTimes;
    std::vector<double> m_clockBindTimes;
    std::vector<double> m_clockEvalTimes;
    std::wstring m_csvFileName;
    std::wstring m_csvFileNamePerIterationSummary;
    std::wstring m_csvFileNamePerIterationResult;
    std::wstring m_folderNamePerIteration;
    std::wstring m_fileNameResultDevice;

    bool m_silent = false;
    bool m_flagGpuDevice = false;

    std::vector<double> m_EvalTime;
    std::vector<double> m_CPUWorkingDiff;
    std::vector<double> m_CPUWorkingStart;
    std::vector<double> m_GPUSharedDiff;
    std::vector<double> m_GPUSharedStart;
    std::vector<double> m_GPUDedicatedDiff;
    std::vector<std::string> m_outputResult;
    std::vector<int> m_outputTensorHash;

#if defined(_AMD64_)
    // PIX markers only work on amd64
    com_ptr<IDXGraphicsAnalysis> m_graphicsAnalysis = nullptr;
#endif
};