#include <Windows.h>
#include "Filehelper.h"
#include "CppUnitTest.h"
#include <processthreadsapi.h>
#include <winnt.h>
#include <Winbase.h>
#include <fstream>
#include <algorithm>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
static int RunProc(LPWSTR commandLine)
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
    return exitCode;
}

namespace WinMLRunnerTest
{
    static const std::wstring CURRENT_PATH = FileHelper::GetModulePath();
    static const std::wstring EXE_PATH = CURRENT_PATH + L"WinMLRunner.exe";
    static const std::wstring INPUT_FOLDER_PATH = CURRENT_PATH + L"test_folder_input";
    static const std::wstring OUTPUT_PATH = CURRENT_PATH + L"test_output.csv";

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
        return std::count(std::istreambuf_iterator<char>(fin), std::istreambuf_iterator<char>(), '\n');
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

	TEST_CLASS(GarbageInputTest)
	{
	public:
        TEST_CLASS_INITIALIZE(SetupClass)
        {
            // Make test_folder_input folder before starting the tests
            std::string mkFolderCommand = "mkdir " + std::string(INPUT_FOLDER_PATH.begin(), INPUT_FOLDER_PATH.end());
            system(mkFolderCommand.c_str());

            std::vector<std::string> models = { "SqueezeNet.onnx", "Add_ImageNet224.onnx" };

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
            std::remove(std::string(OUTPUT_PATH.begin(), OUTPUT_PATH.end()).c_str());
        }

		TEST_METHOD(GarbageInputCpuAndGpu)
		{
            const std::wstring modelPath = CURRENT_PATH + L"SqueezeNet.onnx";
            const std::wstring command = BuildCommand({ EXE_PATH, L"-model", modelPath, L"-output", OUTPUT_PATH, L"-perf" });
            Assert::AreEqual(0, RunProc((wchar_t *)command.c_str()));

            // We need to expect one more line because of the header
            Assert::AreEqual(static_cast<size_t>(3), GetOutputCSVLineCount());
		}

        TEST_METHOD(GarbageInputOnlyCpu)
        {
            const std::wstring modelPath = CURRENT_PATH + L"SqueezeNet.onnx";
            const std::wstring command = BuildCommand({ EXE_PATH, L"-model", modelPath, L"-output", OUTPUT_PATH, L"-perf", L"-CPU" });
            Assert::AreEqual(0, RunProc((wchar_t *)command.c_str()));

            // We need to expect one more line because of the header
            Assert::AreEqual(static_cast<size_t>(2), GetOutputCSVLineCount());
        }

        TEST_METHOD(GarbageInputOnlyGpu)
        {
            const std::wstring modelPath = CURRENT_PATH + L"SqueezeNet.onnx";
            const std::wstring command = BuildCommand({ EXE_PATH, L"-model", modelPath, L"-output", OUTPUT_PATH, L"-perf", L"-GPU" });
            Assert::AreEqual(0, RunProc((wchar_t *)command.c_str()));

            // We need to expect one more line because of the header
            Assert::AreEqual(static_cast<size_t>(2), GetOutputCSVLineCount());
        }

        TEST_METHOD(GarbageInputCpuDeviceCpuBoundRGBImage)
        {
            const std::wstring modelPath = CURRENT_PATH + L"SqueezeNet.onnx";
            const std::wstring command = BuildCommand({ EXE_PATH, L"-model", modelPath, L"-output", OUTPUT_PATH, L"-perf", L"-CPU", L"-CPUBoundInput", L"-RGB" });
            Assert::AreEqual(0, RunProc((wchar_t *)command.c_str()));

            // We need to expect one more line because of the header
            Assert::AreEqual(static_cast<size_t>(2), GetOutputCSVLineCount());
        }

        TEST_METHOD(GarbageInputCpuDeviceCpuBoundBGRImage)
        {
            const std::wstring modelPath = CURRENT_PATH + L"SqueezeNet.onnx";
            const std::wstring command = BuildCommand({ EXE_PATH, L"-model", modelPath, L"-output", OUTPUT_PATH, L"-perf", L"-CPU", L"-CPUBoundInput", L"-BGR" });
            Assert::AreEqual(0, RunProc((wchar_t *)command.c_str()));

            // We need to expect one more line because of the header
            Assert::AreEqual(static_cast<size_t>(2), GetOutputCSVLineCount());
        }

        TEST_METHOD(GarbageInputCpuDeviceCpuBoundTensor)
        {
            const std::wstring modelPath = CURRENT_PATH + L"SqueezeNet.onnx";
            const std::wstring command = BuildCommand({ EXE_PATH, L"-model", modelPath, L"-output", OUTPUT_PATH, L"-perf", L"-CPU", L"-CPUBoundInput", L"-tensor" });
            Assert::AreEqual(0, RunProc((wchar_t *)command.c_str()));

            // We need to expect one more line because of the header
            Assert::AreEqual(static_cast<size_t>(2), GetOutputCSVLineCount());
        }

        TEST_METHOD(GarbageInputCpuDeviceGpuBoundRGBImage)
        {
            const std::wstring modelPath = CURRENT_PATH + L"SqueezeNet.onnx";
            const std::wstring command = BuildCommand({ EXE_PATH, L"-model", modelPath, L"-output", OUTPUT_PATH, L"-perf", L"-CPU", L"-GPUBoundInput", L"-RGB" });
            Assert::AreEqual(0, RunProc((wchar_t *)command.c_str()));

            // We need to expect one more line because of the header
            Assert::AreEqual(static_cast<size_t>(2), GetOutputCSVLineCount());
        }

        TEST_METHOD(GarbageInputCpuDeviceGpuBoundBGRImage)
        {
            const std::wstring modelPath = CURRENT_PATH + L"SqueezeNet.onnx";
            const std::wstring command = BuildCommand({ EXE_PATH, L"-model", modelPath, L"-output", OUTPUT_PATH, L"-perf", L"-CPU", L"-GPUBoundInput", L"-BGR" });
            Assert::AreEqual(0, RunProc((wchar_t *)command.c_str()));

            // We need to expect one more line because of the header
            Assert::AreEqual(static_cast<size_t>(2), GetOutputCSVLineCount());
        }

        TEST_METHOD(GarbageInputCpuDeviceGpuBoundTensor)
        {
            const std::wstring modelPath = CURRENT_PATH + L"SqueezeNet.onnx";
            const std::wstring command = BuildCommand({ EXE_PATH, L"-model", modelPath, L"-output", OUTPUT_PATH, L"-perf", L"-CPU", L"-GPUBoundInput", L"-tensor" });
            Assert::AreEqual(0, RunProc((wchar_t *)command.c_str()));

            // We need to expect one more line because of the header
            Assert::AreEqual(static_cast<size_t>(2), GetOutputCSVLineCount());
        }

        TEST_METHOD(GarbageInputGpuDeviceCpuBoundRGBImage)
        {
            const std::wstring modelPath = CURRENT_PATH + L"SqueezeNet.onnx";
            const std::wstring command = BuildCommand({ EXE_PATH, L"-model", modelPath, L"-output", OUTPUT_PATH, L"-perf", L"-GPU", L"-CPUBoundInput", L"-RGB" });
            Assert::AreEqual(0, RunProc((wchar_t *)command.c_str()));

            // We need to expect one more line because of the header
            Assert::AreEqual(static_cast<size_t>(2), GetOutputCSVLineCount());
        }

        TEST_METHOD(GarbageInputGpuDeviceCpuBoundBGRImage)
        {
            const std::wstring modelPath = CURRENT_PATH + L"SqueezeNet.onnx";
            const std::wstring command = BuildCommand({ EXE_PATH, L"-model", modelPath, L"-output", OUTPUT_PATH, L"-perf", L"-GPU", L"-CPUBoundInput", L"-BGR" });
            Assert::AreEqual(0, RunProc((wchar_t *)command.c_str()));

            // We need to expect one more line because of the header
            Assert::AreEqual(static_cast<size_t>(2), GetOutputCSVLineCount());
        }

        TEST_METHOD(GarbageInputGpuDeviceCpuBoundTensor)
        {
            const std::wstring modelPath = CURRENT_PATH + L"SqueezeNet.onnx";
            const std::wstring command = BuildCommand({ EXE_PATH, L"-model", modelPath, L"-output", OUTPUT_PATH, L"-perf", L"-GPU", L"-CPUBoundInput", L"-tensor" });
            Assert::AreEqual(0, RunProc((wchar_t *)command.c_str()));

            // We need to expect one more line because of the header
            Assert::AreEqual(static_cast<size_t>(2), GetOutputCSVLineCount());
        }

        TEST_METHOD(GarbageInputGpuDeviceGpuBoundRGBImage)
        {
            const std::wstring modelPath = CURRENT_PATH + L"SqueezeNet.onnx";
            const std::wstring command = BuildCommand({ EXE_PATH, L"-model", modelPath, L"-output", OUTPUT_PATH, L"-perf", L"-GPU", L"-GPUBoundInput", L"-RGB" });
            Assert::AreEqual(0, RunProc((wchar_t *)command.c_str()));

            // We need to expect one more line because of the header
            Assert::AreEqual(static_cast<size_t>(2), GetOutputCSVLineCount());
        }

        TEST_METHOD(GarbageInputGpuDeviceGpuBoundBGRImage)
        {
            const std::wstring modelPath = CURRENT_PATH + L"SqueezeNet.onnx";
            const std::wstring command = BuildCommand({ EXE_PATH, L"-model", modelPath, L"-output", OUTPUT_PATH, L"-perf", L"-GPU", L"-GPUBoundInput", L"-BGR" });
            Assert::AreEqual(0, RunProc((wchar_t *)command.c_str()));

            // We need to expect one more line because of the header
            Assert::AreEqual(static_cast<size_t>(2), GetOutputCSVLineCount());
        }

        TEST_METHOD(GarbageInputGpuDeviceGpuBoundTensor)
        {
            const std::wstring modelPath = CURRENT_PATH + L"SqueezeNet.onnx";
            const std::wstring command = BuildCommand({ EXE_PATH, L"-model", modelPath, L"-output", OUTPUT_PATH, L"-perf", L"-GPU", L"-GPUBoundInput", L"-tensor" });
            Assert::AreEqual(0, RunProc((wchar_t *)command.c_str()));

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
                L"-output",
                OUTPUT_PATH,
                L"-perf",
                L"-CPU",
                L"-GPU",
                L"-CPUBoundInput",
                L"-GPUBoundInput",
                L"-RGB",
                L"-BGR",
                L"-tensor"
            });
            Assert::AreEqual(0, RunProc((wchar_t *)command.c_str()));

            // We need to expect one more line because of the header
            Assert::AreEqual(static_cast<size_t>(13), GetOutputCSVLineCount());
        }

        TEST_METHOD(RunAllModelsInFolderGarbageInput)
        {
            const std::wstring command = BuildCommand({ EXE_PATH, L"-folder", INPUT_FOLDER_PATH, L"-output", OUTPUT_PATH, L"-perf" });
            Assert::AreEqual(0, RunProc((wchar_t *)command.c_str()));

            // We need to expect one more line because of the header
            Assert::AreEqual(static_cast<size_t>(5), GetOutputCSVLineCount());
        }

        TEST_METHOD(RunAllModelsInFolderGarbageInputWithAllPermutations)
        {
            const std::wstring command = BuildCommand({
                EXE_PATH,
                L"-folder",
                INPUT_FOLDER_PATH,
                L"-output",
                OUTPUT_PATH,
                L"-perf",
                L"-CPU",
                L"-GPU",
                L"-CPUBoundInput",
                L"-GPUBoundInput",
                L"-RGB",
                L"-BGR",
                L"-tensor"
                });
            Assert::AreEqual(0, RunProc((wchar_t *)command.c_str()));

            // We need to expect one more line because of the header
            Assert::AreEqual(static_cast<size_t>(25), GetOutputCSVLineCount());
        }
	};

    TEST_CLASS(ImageInputTest)
    {
    public:

        TEST_METHOD(ProvidedImageInputCpuAndGpu)
        {
            auto const curPath = FileHelper::GetModulePath();
            std::wstring command = curPath +
                L"./WinMLRunner " + L"-model " + curPath + L"SqueezeNet.onnx "
                + L" -input " + curPath + L"fish.png";
            Assert::AreEqual(0, RunProc((wchar_t *)command.c_str()));
        }

        TEST_METHOD(ProvidedImageInputOnlyCpu)
        {
            auto const curPath = FileHelper::GetModulePath();
            std::wstring command = curPath +
                L"./WinMLRunner " + L"-model " + curPath + L"SqueezeNet.onnx " + L"-CPU"
                + L" -input " + curPath + L"fish.png";
            Assert::AreEqual(0, RunProc((wchar_t *)command.c_str()));
        }

        TEST_METHOD(ProvidedImageInputOnlyGpu)
        {
            auto const curPath = FileHelper::GetModulePath();
            std::wstring command = curPath +
                L"./WinMLRunner " + L"-model " + curPath + L"SqueezeNet.onnx " + L"-GPU"
                + L" -input " + curPath + L"fish.png";
            Assert::AreEqual(0, RunProc((wchar_t *)command.c_str()));
        }

        TEST_METHOD(ProvidedImageBadBinding)
        {
            auto const curPath = FileHelper::GetModulePath();
            std::wstring command = curPath +
                L"./WinMLRunner " + L"-model " + curPath + L"SqueezeNet.onnx "
                + L" -input " + curPath + L"horizontal-crop.png";
            Assert::AreNotEqual(0, RunProc((wchar_t *)command.c_str()));
        }
    };

    TEST_CLASS(CsvInputTest)
    {
    public:
        TEST_METHOD(ProvidedCSVInput)
        {
            auto const curPath = FileHelper::GetModulePath();
            std::wstring command = curPath +
                L"./WinMLRunner " + L"-model " + curPath + L"SqueezeNet.onnx "
                + L" -input " + curPath + L"kitten_224.csv";
            Assert::AreEqual(0, RunProc((wchar_t *)command.c_str()));
        }

        TEST_METHOD(ProvidedCSVBadBinding)
        {
            auto const curPath = FileHelper::GetModulePath();
            std::wstring command = curPath +
                L"./WinMLRunner " + L"-model " + curPath + L"SqueezeNet.onnx "
                + L" -input " + curPath + L"horizontal-crop.csv";
            Assert::AreNotEqual(0, RunProc((wchar_t *)command.c_str()));
        }
    };

    TEST_CLASS(OtherTests)
    {
    public:
        TEST_METHOD(LoadModelFailModelNotFound)
        {
            auto const curPath = FileHelper::GetModulePath();
            std::wstring command = curPath + L"./WinMLRunner " + L"-model invalid_model_name";
            Assert::AreNotEqual(0, RunProc((wchar_t *)command.c_str()));
        }

        TEST_METHOD(TestPrintUsage)
        {
            auto const curPath = FileHelper::GetModulePath();
            std::wstring command = curPath +
                L"./WinMLRunner";
            Assert::AreEqual(0, RunProc((wchar_t *)command.c_str()));
        }
    };
}