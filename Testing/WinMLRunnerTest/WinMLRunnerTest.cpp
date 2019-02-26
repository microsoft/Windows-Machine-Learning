#include <Windows.h>
#include "Filehelper.h"
#include "CppUnitTest.h"
#include <processthreadsapi.h>
#include <winnt.h>
#include <Winbase.h>
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <vector>
#include <direct.h>
#include <iomanip>
#include <codecvt>
#include <locale> 
#include <cmath>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
static HRESULT RunProc(LPWSTR commandLine)
{
    STARTUPINFO SI = { 0 };
    PROCESS_INFORMATION PI = { 0 };
    DWORD CreationFlags = 0;
    SI.cb = sizeof(SI);
    Assert::IsTrue(
        0 != CreateProcess(
            nullptr, commandLine, nullptr, nullptr, FALSE, CreationFlags, nullptr, nullptr, &SI, &PI));
    Assert::AreEqual(WAIT_OBJECT_0, WaitForSingleObject(PI.hProcess, INFINITE));
    DWORD exitCode;
    Assert::IsTrue(0 != GetExitCodeProcess(PI.hProcess, &exitCode));
    CloseHandle(PI.hThread);
    CloseHandle(PI.hProcess);
    return HRESULT_FROM_WIN32(exitCode);
}

namespace WinMLRunnerTest
{
    static const std::wstring CURRENT_PATH = FileHelper::GetModulePath();
    static const std::wstring EXE_PATH = CURRENT_PATH + L"WinMLRunner.exe";
    static const std::wstring INPUT_FOLDER_PATH = CURRENT_PATH + L"test_folder_input";
    static const std::wstring OUTPUT_PATH = CURRENT_PATH + L"test_output.csv";
    static const std::wstring TENSOR_DATA_PATH = CURRENT_PATH + L"\\TestResult";

    static std::wstring BuildCommand(std::initializer_list<std::wstring>&& arguments)
    {
        std::wstring commandLine;

        for (const std::wstring& argument : arguments)
        {
            commandLine += argument + L' ';
        }

        return commandLine;
    }

    static size_t GetOutputCSVLineCount()
    {
        std::ifstream fin;
        fin.open(OUTPUT_PATH);
        return static_cast<size_t>(std::count(std::istreambuf_iterator<char>(fin), std::istreambuf_iterator<char>(), '\n'));
    }

    static void RemoveModelsFromFolder(std::initializer_list<std::string>&& modelList)
    {
        //make test_models folder
        std::string mkFolderCommand = "mkdir " + std::string(INPUT_FOLDER_PATH.begin(), INPUT_FOLDER_PATH.end());
        system(mkFolderCommand.c_str());

        //copy models from list to test_folder_input
        for (auto model : modelList)
        {
            std::string copyCommand = "Copy ";
            copyCommand += model;
            copyCommand += ' ' + std::string(INPUT_FOLDER_PATH.begin(), INPUT_FOLDER_PATH.end());
            system(copyCommand.c_str());
        }
    }

    bool CompareTensors(const std::wstring &expectedOutputTensorFile, const std::wstring &actualOutputTensorFile)
    {
        bool check = false;
        std::ifstream expectedFileStream;
        std::ifstream actualFileStream;
        expectedFileStream.open(expectedOutputTensorFile);
        actualFileStream.open(actualOutputTensorFile);
        std::string actualValue;
        std::string expectedValue;
        if (expectedFileStream.fail() || actualFileStream.fail())
        {
            return false;
        }
        // hard coded, might need to be modified later.
        float relativeTolerance = 0.003f;
        bool isFirstRow = true;
        while (!(expectedFileStream.eof() || actualFileStream.eof()))
        {
            std::getline(expectedFileStream, expectedValue, ',');
            std::getline(expectedFileStream, expectedValue, '\n');
            std::getline(actualFileStream, actualValue, ',');
            std::getline(actualFileStream, actualValue, '\n');
            if (isFirstRow)
            {
                isFirstRow = false;
                continue;
            }
            float actualValueNum = (actualValue == "") ? 0 : std::stof(actualValue);
            float expectedValueNum = (expectedValue == "") ? 0 : std::stof(expectedValue);
            if (std::abs(actualValueNum - expectedValueNum) > relativeTolerance * std::abs(expectedValueNum) + relativeTolerance) //Check if the values are too different.
            {
                return false;
            }
        }
        return true;
    }

    TEST_CLASS(GarbageInputTest)
    {
    public:
        TEST_CLASS_INITIALIZE(SetupClass)
        {
            // Make test_folder_input folder before starting the tests
            std::string mkFolderCommand = "mkdir " + std::string(INPUT_FOLDER_PATH.begin(), INPUT_FOLDER_PATH.end());
            system(mkFolderCommand.c_str());

            std::vector<std::string> models = { "SqueezeNet.onnx", "keras_Add_ImageNet_small.onnx" };

            // Copy models from list to test_folder_input
            for (auto model : models)
            {
                std::string copyCommand = "Copy ";
                copyCommand += model;
                copyCommand += ' ' + std::string(INPUT_FOLDER_PATH.begin(), INPUT_FOLDER_PATH.end());
                system(copyCommand.c_str());
            }
        }

        TEST_CLASS_CLEANUP(CleanupClass)
        {
            // Delete test_folder_input folder after all tests have been run
            std::string copyCommand = "rd /s /q ";
            copyCommand += std::string(INPUT_FOLDER_PATH.begin(), INPUT_FOLDER_PATH.end());
            system(copyCommand.c_str());
        }

        TEST_METHOD_CLEANUP(CleanupMethod)
        {
            // Remove output.csv after each test
            // TODO: this will not work if each test runs in parallel. Use different file path for each test, and clean up at class setup
            std::remove(std::string(OUTPUT_PATH.begin(), OUTPUT_PATH.end()).c_str());
            try {
                std::filesystem::remove_all(std::string(TENSOR_DATA_PATH.begin(), TENSOR_DATA_PATH.end()).c_str());
            }
            catch (const std::filesystem::filesystem_error &) {}
        }
        TEST_METHOD(GarbageInputCpuAndGpu)
        {
            const std::wstring modelPath = CURRENT_PATH + L"SqueezeNet.onnx";
            const std::wstring command = BuildCommand({ EXE_PATH, L"-model", modelPath, L"-PerfOutput", OUTPUT_PATH, L"-perf" });
            Assert::AreEqual(S_OK, RunProc((wchar_t *)command.c_str()));

            // We need to expect one more line because of the header
            Assert::AreEqual(static_cast<size_t>(3), GetOutputCSVLineCount());
        }

        TEST_METHOD(GarbageInputOnlyCpu)
        {
            const std::wstring modelPath = CURRENT_PATH + L"SqueezeNet.onnx";
            const std::wstring command = BuildCommand({ EXE_PATH, L"-model", modelPath, L"-PerfOutput", OUTPUT_PATH, L"-perf", L"-CPU" });
            Assert::AreEqual(S_OK, RunProc((wchar_t *)command.c_str()));

            // We need to expect one more line because of the header
            Assert::AreEqual(static_cast<size_t>(2), GetOutputCSVLineCount());
        }

        TEST_METHOD(GarbageInputOnlyGpu)
        {
            const std::wstring modelPath = CURRENT_PATH + L"SqueezeNet.onnx";
            const std::wstring command = BuildCommand({ EXE_PATH, L"-model", modelPath, L"-PerfOutput", OUTPUT_PATH, L"-perf", L"-GPU" });
            Assert::AreEqual(S_OK, RunProc((wchar_t *)command.c_str()));

            // We need to expect one more line because of the header
            Assert::AreEqual(static_cast<size_t>(2), GetOutputCSVLineCount());
        }

        TEST_METHOD(GarbageInputCpuClientDeviceCpuBoundRGBImage)
        {
            const std::wstring modelPath = CURRENT_PATH + L"SqueezeNet.onnx";
            const std::wstring command = BuildCommand({ EXE_PATH, L"-model", modelPath, L"-PerfOutput", OUTPUT_PATH, L"-perf", L"-CPU", L"-CPUBoundInput", L"-RGB", L"-CreateDeviceOnClient" });
            Assert::AreEqual(S_OK, RunProc((wchar_t *)command.c_str()));

            // We need to expect one more line because of the header
            Assert::AreEqual(static_cast<size_t>(2), GetOutputCSVLineCount());
        }

        TEST_METHOD(GarbageInputCpuWinMLDeviceCpuBoundRGBImage)
        {
            const std::wstring modelPath = CURRENT_PATH + L"SqueezeNet.onnx";
            const std::wstring command = BuildCommand({ EXE_PATH, L"-model", modelPath, L"-PerfOutput", OUTPUT_PATH, L"-perf", L"-CPU", L"-CPUBoundInput", L"-RGB", L"-CreateDeviceInWinML" });
            Assert::AreEqual(S_OK, RunProc((wchar_t *)command.c_str()));

            // We need to expect one more line because of the header
            Assert::AreEqual(static_cast<size_t>(2), GetOutputCSVLineCount());
        }

        TEST_METHOD(GarbageInputCpuClientDeviceCpuBoundBGRImage)
        {
            const std::wstring modelPath = CURRENT_PATH + L"SqueezeNet.onnx";
            const std::wstring command = BuildCommand({ EXE_PATH, L"-model", modelPath, L"-PerfOutput", OUTPUT_PATH, L"-perf", L"-CPU", L"-CPUBoundInput", L"-BGR", L"-CreateDeviceOnClient" });
            Assert::AreEqual(S_OK, RunProc((wchar_t *)command.c_str()));

            // We need to expect one more line because of the header
            Assert::AreEqual(static_cast<size_t>(2), GetOutputCSVLineCount());
        }

        TEST_METHOD(GarbageInputCpuWinMLDeviceCpuBoundBGRImage)
        {
            const std::wstring modelPath = CURRENT_PATH + L"SqueezeNet.onnx";
            const std::wstring command = BuildCommand({ EXE_PATH, L"-model", modelPath, L"-PerfOutput", OUTPUT_PATH, L"-perf", L"-CPU", L"-CPUBoundInput", L"-BGR", L"-CreateDeviceInWinML" });
            Assert::AreEqual(S_OK, RunProc((wchar_t *)command.c_str()));

            // We need to expect one more line because of the header
            Assert::AreEqual(static_cast<size_t>(2), GetOutputCSVLineCount());
        }

        TEST_METHOD(GarbageInputCpuClientDeviceCpuBoundTensor)
        {
            const std::wstring modelPath = CURRENT_PATH + L"SqueezeNet.onnx";
            const std::wstring command = BuildCommand({ EXE_PATH, L"-model", modelPath, L"-PerfOutput", OUTPUT_PATH, L"-perf", L"-CPU", L"-CPUBoundInput", L"-tensor", L"-CreateDeviceOnClient" });
            Assert::AreEqual(S_OK, RunProc((wchar_t *)command.c_str()));

            // We need to expect one more line because of the header
            Assert::AreEqual(static_cast<size_t>(2), GetOutputCSVLineCount());
        }

        TEST_METHOD(GarbageInputCpuWinMLDeviceCpuBoundTensor)
        {
            const std::wstring modelPath = CURRENT_PATH + L"SqueezeNet.onnx";
            const std::wstring command = BuildCommand({ EXE_PATH, L"-model", modelPath, L"-PerfOutput", OUTPUT_PATH, L"-perf", L"-CPU", L"-CPUBoundInput", L"-tensor", L"-CreateDeviceInWinML" });
            Assert::AreEqual(S_OK, RunProc((wchar_t *)command.c_str()));

            // We need to expect one more line because of the header
            Assert::AreEqual(static_cast<size_t>(2), GetOutputCSVLineCount());
        }

        TEST_METHOD(GarbageInputCpuClientDeviceGpuBoundRGBImage)
        {
            const std::wstring modelPath = CURRENT_PATH + L"SqueezeNet.onnx";
            const std::wstring command = BuildCommand({ EXE_PATH, L"-model", modelPath, L"-PerfOutput", OUTPUT_PATH, L"-perf", L"-CPU", L"-GPUBoundInput", L"-RGB", L"-CreateDeviceOnClient" });
            Assert::AreEqual(S_OK, RunProc((wchar_t *)command.c_str()));

            // We need to expect one more line because of the header
            Assert::AreEqual(static_cast<size_t>(2), GetOutputCSVLineCount());
        }

        TEST_METHOD(GarbageInputCpuWinMLDeviceGpuBoundRGBImage)
        {
            const std::wstring modelPath = CURRENT_PATH + L"SqueezeNet.onnx";
            const std::wstring command = BuildCommand({ EXE_PATH, L"-model", modelPath, L"-PerfOutput", OUTPUT_PATH, L"-perf", L"-CPU", L"-GPUBoundInput", L"-RGB", L"-CreateDeviceInWinML" });
            Assert::AreEqual(S_OK, RunProc((wchar_t *)command.c_str()));

            // We need to expect one more line because of the header
            Assert::AreEqual(static_cast<size_t>(2), GetOutputCSVLineCount());
        }

        TEST_METHOD(GarbageInputCpuClientDeviceGpuBoundBGRImage)
        {
            const std::wstring modelPath = CURRENT_PATH + L"SqueezeNet.onnx";
            const std::wstring command = BuildCommand({ EXE_PATH, L"-model", modelPath, L"-PerfOutput", OUTPUT_PATH, L"-perf", L"-CPU", L"-GPUBoundInput", L"-BGR", L"-CreateDeviceOnClient" });
            Assert::AreEqual(S_OK, RunProc((wchar_t *)command.c_str()));

            // We need to expect one more line because of the header
            Assert::AreEqual(static_cast<size_t>(2), GetOutputCSVLineCount());
        }

        TEST_METHOD(GarbageInputCpuWinMLDeviceGpuBoundBGRImage)
        {
            const std::wstring modelPath = CURRENT_PATH + L"SqueezeNet.onnx";
            const std::wstring command = BuildCommand({ EXE_PATH, L"-model", modelPath, L"-PerfOutput", OUTPUT_PATH, L"-perf", L"-CPU", L"-GPUBoundInput", L"-BGR", L"-CreateDeviceInWinML" });
            Assert::AreEqual(S_OK, RunProc((wchar_t *)command.c_str()));

            // We need to expect one more line because of the header
            Assert::AreEqual(static_cast<size_t>(2), GetOutputCSVLineCount());
        }

        TEST_METHOD(GarbageInputCpuClientDeviceGpuBoundTensor)
        {
            const std::wstring modelPath = CURRENT_PATH + L"SqueezeNet.onnx";
            const std::wstring command = BuildCommand({ EXE_PATH, L"-model", modelPath, L"-PerfOutput", OUTPUT_PATH, L"-perf", L"-CPU", L"-GPUBoundInput", L"-tensor", L"-CreateDeviceOnClient" });
            Assert::AreEqual(S_OK, RunProc((wchar_t *)command.c_str()));

            // We need to expect one more line because of the header
            Assert::AreEqual(static_cast<size_t>(2), GetOutputCSVLineCount());
        }

        TEST_METHOD(GarbageInputCpuWinMLDeviceGpuBoundTensor)
        {
            const std::wstring modelPath = CURRENT_PATH + L"SqueezeNet.onnx";
            const std::wstring command = BuildCommand({ EXE_PATH, L"-model", modelPath, L"-PerfOutput", OUTPUT_PATH, L"-perf", L"-CPU", L"-GPUBoundInput", L"-tensor", L"-CreateDeviceInWinML" });
            Assert::AreEqual(S_OK, RunProc((wchar_t *)command.c_str()));

            // We need to expect one more line because of the header
            Assert::AreEqual(static_cast<size_t>(2), GetOutputCSVLineCount());
        }

        TEST_METHOD(GarbageInputGpuClientDeviceCpuBoundRGBImage)
        {
            const std::wstring modelPath = CURRENT_PATH + L"SqueezeNet.onnx";
            const std::wstring command = BuildCommand({ EXE_PATH, L"-model", modelPath, L"-PerfOutput", OUTPUT_PATH, L"-perf", L"-GPU", L"-CPUBoundInput", L"-RGB", L"-CreateDeviceOnClient" });
            Assert::AreEqual(S_OK, RunProc((wchar_t *)command.c_str()));

            // We need to expect one more line because of the header
            Assert::AreEqual(static_cast<size_t>(2), GetOutputCSVLineCount());
        }

        TEST_METHOD(GarbageInputGpuWinMLDeviceCpuBoundRGBImage)
        {
            const std::wstring modelPath = CURRENT_PATH + L"SqueezeNet.onnx";
            const std::wstring command = BuildCommand({ EXE_PATH, L"-model", modelPath, L"-PerfOutput", OUTPUT_PATH, L"-perf", L"-GPU", L"-CPUBoundInput", L"-RGB", L"-CreateDeviceInWinML" });
            Assert::AreEqual(S_OK, RunProc((wchar_t *)command.c_str()));

            // We need to expect one more line because of the header
            Assert::AreEqual(static_cast<size_t>(2), GetOutputCSVLineCount());
        }

        TEST_METHOD(GarbageInputGpuClientDeviceCpuBoundBGRImage)
        {
            const std::wstring modelPath = CURRENT_PATH + L"SqueezeNet.onnx";
            const std::wstring command = BuildCommand({ EXE_PATH, L"-model", modelPath, L"-PerfOutput", OUTPUT_PATH, L"-perf", L"-GPU", L"-CPUBoundInput", L"-BGR", L"-CreateDeviceOnClient" });
            Assert::AreEqual(S_OK, RunProc((wchar_t *)command.c_str()));

            // We need to expect one more line because of the header
            Assert::AreEqual(static_cast<size_t>(2), GetOutputCSVLineCount());
        }

        TEST_METHOD(GarbageInputGpuWinMLDeviceCpuBoundBGRImage)
        {
            const std::wstring modelPath = CURRENT_PATH + L"SqueezeNet.onnx";
            const std::wstring command = BuildCommand({ EXE_PATH, L"-model", modelPath, L"-PerfOutput", OUTPUT_PATH, L"-perf", L"-GPU", L"-CPUBoundInput", L"-BGR", L"-CreateDeviceInWinML" });
            Assert::AreEqual(S_OK, RunProc((wchar_t *)command.c_str()));

            // We need to expect one more line because of the header
            Assert::AreEqual(static_cast<size_t>(2), GetOutputCSVLineCount());
        }

        TEST_METHOD(GarbageInputGpuClientDeviceCpuBoundTensor)
        {
            const std::wstring modelPath = CURRENT_PATH + L"SqueezeNet.onnx";
            const std::wstring command = BuildCommand({ EXE_PATH, L"-model", modelPath, L"-PerfOutput", OUTPUT_PATH, L"-perf", L"-GPU", L"-CPUBoundInput", L"-tensor", L"-CreateDeviceOnClient" });
            Assert::AreEqual(S_OK, RunProc((wchar_t *)command.c_str()));

            // We need to expect one more line because of the header
            Assert::AreEqual(static_cast<size_t>(2), GetOutputCSVLineCount());
        }

        TEST_METHOD(GarbageInputGpuWinMLDeviceCpuBoundTensor)
        {
            const std::wstring modelPath = CURRENT_PATH + L"SqueezeNet.onnx";
            const std::wstring command = BuildCommand({ EXE_PATH, L"-model", modelPath, L"-PerfOutput", OUTPUT_PATH, L"-perf", L"-GPU", L"-CPUBoundInput", L"-tensor", L"-CreateDeviceInWinML" });
            Assert::AreEqual(S_OK, RunProc((wchar_t *)command.c_str()));

            // We need to expect one more line because of the header
            Assert::AreEqual(static_cast<size_t>(2), GetOutputCSVLineCount());
        }

        TEST_METHOD(GarbageInputGpuClientDeviceGpuBoundRGBImage)
        {
            const std::wstring modelPath = CURRENT_PATH + L"SqueezeNet.onnx";
            const std::wstring command = BuildCommand({ EXE_PATH, L"-model", modelPath, L"-PerfOutput", OUTPUT_PATH, L"-perf", L"-GPU", L"-GPUBoundInput", L"-RGB", L"-CreateDeviceOnClient" });
            Assert::AreEqual(S_OK, RunProc((wchar_t *)command.c_str()));

            // We need to expect one more line because of the header
            Assert::AreEqual(static_cast<size_t>(2), GetOutputCSVLineCount());
        }

        TEST_METHOD(GarbageInputGpuWinMLDeviceGpuBoundRGBImage)
        {
            const std::wstring modelPath = CURRENT_PATH + L"SqueezeNet.onnx";
            const std::wstring command = BuildCommand({ EXE_PATH, L"-model", modelPath, L"-PerfOutput", OUTPUT_PATH, L"-perf", L"-GPU", L"-GPUBoundInput", L"-RGB", L"-CreateDeviceInWinML" });
            Assert::AreEqual(S_OK, RunProc((wchar_t *)command.c_str()));

            // We need to expect one more line because of the header
            Assert::AreEqual(static_cast<size_t>(2), GetOutputCSVLineCount());
        }

        TEST_METHOD(GarbageInputGpuClientDeviceGpuBoundBGRImage)
        {
            const std::wstring modelPath = CURRENT_PATH + L"SqueezeNet.onnx";
            const std::wstring command = BuildCommand({ EXE_PATH, L"-model", modelPath, L"-PerfOutput", OUTPUT_PATH, L"-perf", L"-GPU", L"-GPUBoundInput", L"-BGR", L"-CreateDeviceOnClient" });
            Assert::AreEqual(S_OK, RunProc((wchar_t *)command.c_str()));

            // We need to expect one more line because of the header
            Assert::AreEqual(static_cast<size_t>(2), GetOutputCSVLineCount());
        }

        TEST_METHOD(GarbageInputGpuWinMLDeviceGpuBoundBGRImage)
        {
            const std::wstring modelPath = CURRENT_PATH + L"SqueezeNet.onnx";
            const std::wstring command = BuildCommand({ EXE_PATH, L"-model", modelPath, L"-PerfOutput", OUTPUT_PATH, L"-perf", L"-GPU", L"-GPUBoundInput", L"-BGR", L"-CreateDeviceInWinML" });
            Assert::AreEqual(S_OK, RunProc((wchar_t *)command.c_str()));

            // We need to expect one more line because of the header
            Assert::AreEqual(static_cast<size_t>(2), GetOutputCSVLineCount());
        }

        TEST_METHOD(GarbageInputGpuClientDeviceGpuBoundTensor)
        {
            const std::wstring modelPath = CURRENT_PATH + L"SqueezeNet.onnx";
            const std::wstring command = BuildCommand({ EXE_PATH, L"-model", modelPath, L"-PerfOutput", OUTPUT_PATH, L"-perf", L"-GPU", L"-GPUBoundInput", L"-tensor", L"-CreateDeviceOnClient" });
            Assert::AreEqual(S_OK, RunProc((wchar_t *)command.c_str()));

            // We need to expect one more line because of the header
            Assert::AreEqual(static_cast<size_t>(2), GetOutputCSVLineCount());
        }

        TEST_METHOD(GarbageInputGpuWinMLDeviceGpuBoundTensor)
        {
            const std::wstring modelPath = CURRENT_PATH + L"SqueezeNet.onnx";
            const std::wstring command = BuildCommand({ EXE_PATH, L"-model", modelPath, L"-PerfOutput", OUTPUT_PATH, L"-perf", L"-GPU", L"-GPUBoundInput", L"-tensor", L"-CreateDeviceInWinML" });
            Assert::AreEqual(S_OK, RunProc((wchar_t *)command.c_str()));

            // We need to expect one more line because of the header
            Assert::AreEqual(static_cast<size_t>(2), GetOutputCSVLineCount());
        }

        TEST_METHOD(GarbageInputAllPermutations)
        {
            const std::wstring modelPath = CURRENT_PATH + L"SqueezeNet.onnx";
            const std::wstring command = BuildCommand({
                EXE_PATH,
                L"-model",
                modelPath,
                L"-PerfOutput",
                OUTPUT_PATH,
                L"-perf",
                L"-CPU",
                L"-GPU",
                L"-CreateDeviceOnClient",
                L"-CreateDeviceInWinML",
                L"-CPUBoundInput",
                L"-GPUBoundInput",
                L"-RGB",
                L"-BGR",
                L"-tensor"
            });
            Assert::AreEqual(S_OK, RunProc((wchar_t *)command.c_str()));

            // We need to expect one more line because of the header
            Assert::AreEqual(static_cast<size_t>(25), GetOutputCSVLineCount());
        }

        TEST_METHOD(RunAllModelsInFolderGarbageInput)
        {
            const std::wstring command = BuildCommand({ EXE_PATH, L"-folder", INPUT_FOLDER_PATH, L"-PerfOutput", OUTPUT_PATH, L"-perf" });
            Assert::AreEqual(S_OK, RunProc((wchar_t *)command.c_str()));

            // We need to expect one more line because of the header
            Assert::AreEqual(static_cast<size_t>(5), GetOutputCSVLineCount());
        }

        TEST_METHOD(RunAllModelsInFolderGarbageInputWithAllPermutations)
        {
            const std::wstring command = BuildCommand({
                EXE_PATH,
                L"-folder",
                INPUT_FOLDER_PATH,
                L"-PerfOutput",
                OUTPUT_PATH,
                L"-perf",
                L"-CPU",
                L"-GPU",
                L"-CreateDeviceOnClient",
                L"-CreateDeviceInWinML",
                L"-CPUBoundInput",
                L"-GPUBoundInput",
                L"-RGB",
                L"-BGR",
                L"-tensor"
                });
            Assert::AreEqual(S_OK, RunProc((wchar_t *)command.c_str()));

            // We need to expect one more line because of the header
            Assert::AreEqual(static_cast<size_t>(49), GetOutputCSVLineCount());
        }
        TEST_METHOD(GarbageInputOnlyGpuSaveTensor)
        {
            const std::wstring modelPath = CURRENT_PATH + L"SqueezeNet.onnx";
            const std::wstring command = BuildCommand({ EXE_PATH, L"-model", modelPath, L"-PerfOutput", OUTPUT_PATH, L"-SaveTensorData", L"First", TENSOR_DATA_PATH, L"-GPU"});
            Assert::AreEqual(S_OK, RunProc((wchar_t *)command.c_str()));
            Assert::AreEqual(true, CompareTensors(L"OutputTensorData\\Squeezenet_garbage_input_GPU.csv", TENSOR_DATA_PATH + L"\\softmaxout_1GpuIteration1.csv"));
        }
        TEST_METHOD(GarbageInputOnlyCpuSaveTensor)
        {
            const std::wstring modelPath = CURRENT_PATH + L"SqueezeNet.onnx";
            const std::wstring command = BuildCommand({ EXE_PATH, L"-model", modelPath, L"-PerfOutput", OUTPUT_PATH, L"-SaveTensorData", L"First", TENSOR_DATA_PATH, L"-CPU" });
            Assert::AreEqual(S_OK, RunProc((wchar_t *)command.c_str()));
            Assert::AreEqual(true, CompareTensors(L"OutputTensorData\\Squeezenet_garbage_input_CPU.csv", TENSOR_DATA_PATH + L"\\softmaxout_1CpuIteration1.csv"));
        }

        TEST_METHOD(GarbageInputOnlyGpuSaveTensorFp16)
        {
            const std::wstring modelPath = CURRENT_PATH + L"SqueezeNet_fp16.onnx";
            const std::wstring command = BuildCommand({ EXE_PATH, L"-model", modelPath, L"-PerfOutput", OUTPUT_PATH, L"-SaveTensorData", L"First", TENSOR_DATA_PATH, L"-GPU" });
            Assert::AreEqual(S_OK, RunProc((wchar_t *)command.c_str()));
            Assert::AreEqual(true, CompareTensors(L"OutputTensorData\\Squeezenet_fp16_garbage_input_GPU.csv", TENSOR_DATA_PATH + L"\\softmaxout_1GpuIteration1.csv"));
        }
        TEST_METHOD(GarbageInputOnlyCpuSaveTensorFp16)
        {
            const std::wstring modelPath = CURRENT_PATH + L"SqueezeNet_fp16.onnx";
            const std::wstring command = BuildCommand({ EXE_PATH, L"-model", modelPath, L"-PerfOutput", OUTPUT_PATH, L"-SaveTensorData", L"First", TENSOR_DATA_PATH, L"-CPU" });
            Assert::AreEqual(S_OK, RunProc((wchar_t *)command.c_str()));
            Assert::AreEqual(true, CompareTensors(L"OutputTensorData\\Squeezenet_fp16_garbage_input_CPU.csv", TENSOR_DATA_PATH + L"\\softmaxout_1CpuIteration1.csv"));
        }
    };

    TEST_CLASS(ImageInputTest)
    {
    public:
        TEST_METHOD_CLEANUP(CleanupMethod)
        {
            // Remove output.csv after each test
            // TODO: this will not work if each test runs in parallel. Use different file path for each test, and clean up at class setup
            std::remove(std::string(OUTPUT_PATH.begin(), OUTPUT_PATH.end()).c_str());
            try {
                std::filesystem::remove_all(std::string(TENSOR_DATA_PATH.begin(), TENSOR_DATA_PATH.end()).c_str());
            }
            catch (const std::filesystem::filesystem_error &) {}
        }
        TEST_METHOD(ProvidedImageInputCpuAndGpu)
        {
            const std::wstring modelPath = CURRENT_PATH + L"SqueezeNet.onnx";
            const std::wstring inputPath = CURRENT_PATH + L"fish.png";
            const std::wstring command = BuildCommand({ EXE_PATH, L"-model ", modelPath, L"-input", inputPath });
        }

        TEST_METHOD(ProvidedImageInputOnlyCpu)
        {
            const std::wstring modelPath = CURRENT_PATH + L"SqueezeNet.onnx";
            const std::wstring inputPath = CURRENT_PATH + L"fish.png";
            const std::wstring command = BuildCommand({ EXE_PATH, L"-model ", modelPath, L"-input", inputPath, L"-CPU" });
            Assert::AreEqual(S_OK, RunProc((wchar_t *)command.c_str()));
        }

        TEST_METHOD(ProvidedImageInputOnlyGpu)
        {
            const std::wstring modelPath = CURRENT_PATH + L"SqueezeNet.onnx";
            const std::wstring inputPath = CURRENT_PATH + L"fish.png";
            const std::wstring command = BuildCommand({ EXE_PATH, L"-model ", modelPath, L"-input", inputPath, L"-GPU" });
            Assert::AreEqual(S_OK, RunProc((wchar_t *)command.c_str()));
        }

        TEST_METHOD(AutoScaleImage)
        {
            const std::wstring modelPath = CURRENT_PATH + L"SqueezeNet.onnx";
            const std::wstring inputPath = CURRENT_PATH + L"fish_112.png";
            const std::wstring command = BuildCommand({ EXE_PATH, L"-model ", modelPath, L"-input", inputPath, L"-autoScale", L"Cubic" });
            Assert::AreEqual(S_OK, RunProc((wchar_t*)command.c_str()));
        }
        TEST_METHOD(ProvidedImageInputOnlyGpuSaveTensor)
        {
            const std::wstring modelPath = CURRENT_PATH + L"SqueezeNet.onnx";
            const std::wstring inputPath = CURRENT_PATH + L"fish.png";
            const std::wstring command = BuildCommand({ EXE_PATH, L"-model ", modelPath, L"-input", inputPath,
                L"-SaveTensorData", L"First", TENSOR_DATA_PATH, L"-GPU" });
            Assert::AreEqual(S_OK, RunProc((wchar_t*)command.c_str()));
            Assert::AreEqual(true, CompareTensors(L"OutputTensorData\\Squeezenet_fish_input_GPU.csv", TENSOR_DATA_PATH + L"\\softmaxout_1GpuIteration1.csv"));
        }
        TEST_METHOD(ProvidedImageInputOnlyCpuSaveTensor)
        {
            const std::wstring modelPath = CURRENT_PATH + L"SqueezeNet.onnx";
            const std::wstring inputPath = CURRENT_PATH + L"fish.png";
            const std::wstring command = BuildCommand({ EXE_PATH, L"-model ", modelPath, L"-input", inputPath,
                L"-SaveTensorData", L"First", TENSOR_DATA_PATH, L"-CPU" });
            Assert::AreEqual(S_OK, RunProc((wchar_t*)command.c_str()));
            Assert::AreEqual(true, CompareTensors(L"OutputTensorData\\Squeezenet_fish_input_CPU.csv", TENSOR_DATA_PATH + L"\\softmaxout_1CpuIteration1.csv"));
        }
        TEST_METHOD(ProvidedImageInputOnlyGpuSaveTensorFp16)
        {
            const std::wstring modelPath = CURRENT_PATH + L"SqueezeNet_fp16.onnx";
            const std::wstring inputPath = CURRENT_PATH + L"fish.png";
            const std::wstring command = BuildCommand({ EXE_PATH, L"-model ", modelPath, L"-input", inputPath,
                L"-SaveTensorData", L"First", TENSOR_DATA_PATH, L"-GPU" });
            Assert::AreEqual(S_OK, RunProc((wchar_t*)command.c_str()));
            Assert::AreEqual(true, CompareTensors(L"OutputTensorData\\Squeezenet_fp16_fish_input_GPU.csv", TENSOR_DATA_PATH + L"\\softmaxout_1GpuIteration1.csv"));
        }
        TEST_METHOD(ProvidedImageInputOnlyCpuSaveTensorFp16)
        {
            const std::wstring modelPath = CURRENT_PATH + L"SqueezeNet_fp16.onnx";
            const std::wstring inputPath = CURRENT_PATH + L"fish.png";
            const std::wstring command = BuildCommand({ EXE_PATH, L"-model ", modelPath, L"-input", inputPath,
                L"-SaveTensorData", L"First", TENSOR_DATA_PATH, L"-CPU" });
            Assert::AreEqual(S_OK, RunProc((wchar_t*)command.c_str()));
            Assert::AreEqual(true, CompareTensors(L"OutputTensorData\\Squeezenet_fp16_fish_input_CPU.csv", TENSOR_DATA_PATH + L"\\softmaxout_1CpuIteration1.csv"));
        }
    };

    TEST_CLASS(CsvInputTest)
    {
    public:

        TEST_METHOD_CLEANUP(CleanupMethod)
        {
            // Remove output.csv after each test
            // TODO: this will not work if each test runs in parallel. Use different file path for each test, and clean up at class setup
            std::remove(std::string(OUTPUT_PATH.begin(), OUTPUT_PATH.end()).c_str());
            try {
                std::filesystem::remove_all(std::string(TENSOR_DATA_PATH.begin(), TENSOR_DATA_PATH.end()).c_str());
            }
            catch (const std::filesystem::filesystem_error &) {}
        }
        TEST_METHOD(ProvidedCSVInput)
        {
            const std::wstring modelPath = CURRENT_PATH + L"SqueezeNet.onnx";
            const std::wstring inputPath = CURRENT_PATH + L"kitten_224.csv";
            const std::wstring command = BuildCommand({ EXE_PATH, L"-model", modelPath, L"-input", inputPath });
            Assert::AreEqual(S_OK, RunProc((wchar_t *)command.c_str()));
        }
        TEST_METHOD(ProvidedCSVBadBinding)
        {
            const std::wstring modelPath = CURRENT_PATH + L"SqueezeNet.onnx";
            const std::wstring inputPath = CURRENT_PATH + L"horizontal-crop.csv";
            const std::wstring command = BuildCommand({ EXE_PATH, L"-model", modelPath, L"-input", inputPath });
            Assert::AreEqual(HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER), RunProc((wchar_t *)command.c_str()));
        }
        TEST_METHOD(ProvidedCSVInputGPUSaveTensor)
        {
            const std::wstring modelPath = CURRENT_PATH + L"SqueezeNet.onnx";
            const std::wstring inputPath = CURRENT_PATH + L"fish.csv";
            const std::wstring command = BuildCommand({ EXE_PATH, L"-model", modelPath, L"-input", inputPath, L"-SaveTensorData",
                L"First", TENSOR_DATA_PATH, L"-GPU" });
            Assert::AreEqual(S_OK, RunProc((wchar_t *)command.c_str()));
            Assert::AreEqual(true, CompareTensors(L"OutputTensorData\\Squeezenet_fish_input_GPU.csv", TENSOR_DATA_PATH + L"\\softmaxout_1GpuIteration1.csv"));
        }
        TEST_METHOD(ProvidedCSVInputCPUSaveTensor)
        {
            const std::wstring modelPath = CURRENT_PATH + L"SqueezeNet.onnx";
            const std::wstring inputPath = CURRENT_PATH + L"fish.csv";
            const std::wstring command = BuildCommand({ EXE_PATH, L"-model", modelPath, L"-input", inputPath, L"-SaveTensorData",
                L"First", TENSOR_DATA_PATH, L"-CPU" });
            Assert::AreEqual(S_OK, RunProc((wchar_t *)command.c_str()));
            Assert::AreEqual(true, CompareTensors(L"OutputTensorData\\Squeezenet_fish_input_CPU.csv", TENSOR_DATA_PATH + L"\\softmaxout_1CpuIteration1.csv"));
        }
        TEST_METHOD(ProvidedCSVInputGPUSaveTensorFp16)
        {
            const std::wstring modelPath = CURRENT_PATH + L"SqueezeNet_fp16.onnx";
            const std::wstring inputPath = CURRENT_PATH + L"fish.csv";
            const std::wstring command = BuildCommand({ EXE_PATH, L"-model", modelPath, L"-input", inputPath, L"-SaveTensorData",
                L"First", TENSOR_DATA_PATH, L"-GPU" });
            Assert::AreEqual(S_OK, RunProc((wchar_t *)command.c_str()));
            Assert::AreEqual(true, CompareTensors(L"OutputTensorData\\Squeezenet_fp16_fish_input_GPU.csv", TENSOR_DATA_PATH + L"\\softmaxout_1GpuIteration1.csv"));
        }
        TEST_METHOD(ProvidedCSVInputCPUSaveTensorFp16)
        {
            const std::wstring modelPath = CURRENT_PATH + L"SqueezeNet_fp16.onnx";
            const std::wstring inputPath = CURRENT_PATH + L"fish.csv";
            const std::wstring command = BuildCommand({ EXE_PATH, L"-model", modelPath, L"-input", inputPath, L"-SaveTensorData",
                L"First", TENSOR_DATA_PATH, L"-CPU" });
            Assert::AreEqual(S_OK, RunProc((wchar_t *)command.c_str()));
            Assert::AreEqual(true, CompareTensors(L"OutputTensorData\\Squeezenet_fp16_fish_input_CPU.csv", TENSOR_DATA_PATH + L"\\softmaxout_1CpuIteration1.csv"));
        }
    };

    TEST_CLASS(OtherTests)
    {
    public:
        TEST_METHOD(LoadModelFailModelNotFound)
        {
            const std::wstring command = BuildCommand({ EXE_PATH, L"-model", L"invalid_model_name" });
            Assert::AreEqual(HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), RunProc((wchar_t *)command.c_str()));
        }

        TEST_METHOD(TestPrintUsage)
        {
            const std::wstring command = BuildCommand({ EXE_PATH });
            Assert::AreEqual(S_OK, RunProc((wchar_t *)command.c_str()));
        }
        
        /* Commenting out test until WinMLRunnerDLL.dll is properly written and ABI friendly
        TEST_METHOD(TestWinMLRunnerDllLinking)
        {
            // Before running rest of test, make sure that this file doesn't exist or 
            // else renaming WinMLRunnerDLL.dll won't execute properly.
            remove("WinMLRunnerDLL_renamed");

            //Run DLL Linked Executable and check if success
            const std::wstring modelPath = CURRENT_PATH + L"SqueezeNet.onnx";
            const std::wstring dllPath = CURRENT_PATH + L"WinMLRunnerDLL.dll";
            const std::wstring command = BuildCommand({ L"WinMLRunner_Link_DLL.exe",  L"-model", modelPath });
            Assert::AreEqual(S_OK, RunProc((wchar_t *)command.c_str()));

            //Rename WinMLRunnerDLL and then run DLL Linked Executable and check if failed
            rename("WinMLRunnerDLL.dll", "WinMLRunnerDLL_renamed");
            Assert::AreEqual(static_cast<HRESULT>(STATUS_DLL_NOT_FOUND), RunProc((wchar_t *)command.c_str()));

            //rename back to original naming
            rename("WinMLRunnerDLL_renamed", "WinMLRunnerDLL.dll");
        }
        */
    };
}