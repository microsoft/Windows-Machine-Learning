#include "Common.h"
#include "OutputHelper.h"
#include "ModelBinding.h"
#include "BindingUtilities.h"
#include "Stopwatch.h"
#include "CommandLineArgs.h"
#include <filesystem>

#define CheckHr(expr, errorMsg) hr = (expr); if (FAILED(hr)) { WriteErrorMsg(hr, errorMsg); return 1; }

Profiler<WINML_MODEL_TEST_PERF> g_Profiler;
int g_GarbageRuns = 10;
// Loads, binds, and evaluates the user-specified model and outputs the GPU/CPU and
// wall-clock times(in ms) for each step to the command line.
void EvaluateModel(CommandLineArgs args, std::wstring modelName, OutputHelper * output, LearningModelDeviceKind deviceKind)
{
    Stopwatch timer;
    output->PrintModelInfo(modelName, deviceKind);

    WINML_PROFILING_START(g_Profiler, WINML_MODEL_TEST_PERF::LOAD_MODEL);
    timer.Click();

    LearningModel model = nullptr;

        try
        {
            model = LearningModel::LoadFromFilePath(args.ModelPath());
        }
        catch (const std::wstring &msg)
        {
            WriteErrorMsg(msg);
            return;
        }
        WINML_PROFILING_STOP(g_Profiler, WINML_MODEL_TEST_PERF::LOAD_MODEL);
        timer.Click();
        output->m_clockLoadTime = timer.GetElapsedMilliseconds();

    LearningModelSession session(model, LearningModelDevice(deviceKind));
    LearningModelBinding binding(session);

    // Initialize model input and bind garbage data.
    WINML_PROFILING_START(g_Profiler, WINML_MODEL_TEST_PERF::BIND_VALUE);
    timer.Click();
    try
    {
        BindingUtilities::BindGarbageDataToContext(binding, model);
    }
    catch (const std::wstring &msg)
    {
        WriteErrorMsg(msg);
        return;
    }
    timer.Click();
    WINML_PROFILING_STOP(g_Profiler, WINML_MODEL_TEST_PERF::BIND_VALUE);
    output->m_clockBindTime = timer.GetElapsedMilliseconds();

    for (int i = 0; i < g_GarbageRuns; i++) {
        auto result = session.Evaluate(binding, L"");
    }
    for (UINT i = 0; i < args.NumIterations(); i++)
    {
        WINML_PROFILING_START(g_Profiler, WINML_MODEL_TEST_PERF::EVAL_MODEL);
        timer.Click();
        auto result = session.Evaluate(binding, L"");
        timer.Click();
        WINML_PROFILING_STOP(g_Profiler, WINML_MODEL_TEST_PERF::EVAL_MODEL);
        output->m_clockEvalTimes.push_back(timer.GetElapsedMilliseconds());
    }

    output->PrintWallClockTimes(args.NumIterations());
    if (deviceKind == LearningModelDeviceKind::DirectX)
    {
        output->PrintGPUTimes(g_Profiler, args.NumIterations());
    }
    else
    {
        output->PrintCPUTimes(g_Profiler, args.NumIterations());
    }
    g_Profiler.Reset();
}

void EvaluateModelsInDirectory(CommandLineArgs args, OutputHelper * output)
{
    std::wstring folderPath = args.FolderPath();
    for (auto & it : std::filesystem::directory_iterator(args.FolderPath()))
    {
        std::string path = it.path().string();
        if (it.path().string().find(".onnx") != std::string::npos ||
            it.path().string().find(".pb") != std::string::npos)
        {
            std::wstring fileName;
            fileName.assign(path.begin(), path.end());
            args.SetModelPath(fileName);
            try
            {
                if (args.UseCPUandGPU() || args.UseGPU())
                {
                    EvaluateModel(args, args.ModelPath(), output, LearningModelDeviceKind::DirectX);
                }
                if (args.UseCPUandGPU() || args.UseCPU())
                {
                    EvaluateModel(args, args.ModelPath(), output, LearningModelDeviceKind::Cpu);
                }
                output->WritePerformanceDataToCSV(g_Profiler, args, fileName);
                output->Reset();
            }
            catch (const std::wstring &msg)
            {
                WriteErrorMsg(msg);
                continue;
            }
        }
    }
}

int main(int argc, char** argv)
{
    CommandLineArgs args;
    OutputHelper output;

    winrt::init_apartment();
    output.PrintHardwareInfo();
    g_Profiler.Enable();

    std::wstring csvFileName = args.CsvFileName();
    if (csvFileName.empty())
    {
        output.SetDefaultCSVFileName();
    }
    else 
    {
        output.m_csvFileName = csvFileName;
    }
    if (!args.ModelPath().empty())
    {
    
        if (args.UseCPUandGPU() || args.UseGPU())
        {
            EvaluateModel(args, args.ModelPath(), &output, LearningModelDeviceKind::DirectX);
        }
        if (args.UseCPUandGPU() || args.UseCPU())
        {
            EvaluateModel(args, args.ModelPath(), &output, LearningModelDeviceKind::Cpu);
        }
        output.WritePerformanceDataToCSV(g_Profiler, args, args.ModelPath());
        output.Reset();
    }
    else if (!args.FolderPath().empty())
    {
        EvaluateModelsInDirectory(args, &output);
    }
    return 0;
}