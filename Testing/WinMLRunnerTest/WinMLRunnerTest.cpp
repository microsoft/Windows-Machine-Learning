#include <Windows.h>
#include "Filehelper.h"
#include "CppUnitTest.h"
#include <processthreadsapi.h>
#include <winnt.h>
#include <Winbase.h>

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
	TEST_CLASS(GarbageInputTest)
	{
	public:
		TEST_METHOD(GarbageInputCpuAndGpu)
		{
            auto const curPath = FileHelper::GetModulePath();
            std::wstring command = curPath +
                L"./WinMLRunner " + L"-model " + curPath + L"SqueezeNet.onnx";
            Assert::AreEqual(0 , RunProc((wchar_t *)command.c_str()));
		}

        TEST_METHOD(GarbageInputOnlyCpu)
        {
            auto const curPath = FileHelper::GetModulePath();
            std::wstring command = curPath +
                L"./WinMLRunner " + L"-model " + curPath + L"SqueezeNet.onnx " + L"-CPU";
            Assert::AreEqual(0, RunProc((wchar_t *)command.c_str()));
        }

        TEST_METHOD(GarbageInputOnlyGpu)
        {
            auto const curPath = FileHelper::GetModulePath();
            std::wstring command = curPath +
                L"./WinMLRunner " + L"-model " + curPath + L"SqueezeNet.onnx " + L"-GPU";
            Assert::AreEqual(0, RunProc((wchar_t *)command.c_str()));
        }

        TEST_METHOD(GarbageInputCpuDeviceCpuBoundRGBImage)
        {
            auto const curPath = FileHelper::GetModulePath();
            std::wstring command = curPath +
                L"./WinMLRunner " + L"-model " + curPath + L"SqueezeNet.onnx " + L"-CPU " + L"-CPUBoundInput " + L"-RGB ";
            Assert::AreEqual(0, RunProc((wchar_t *)command.c_str()));
        }

        TEST_METHOD(GarbageInputCpuDeviceCpuBoundBGRImage)
        {
            auto const curPath = FileHelper::GetModulePath();
            std::wstring command = curPath +
                L"./WinMLRunner " + L"-model " + curPath + L"SqueezeNet.onnx " + L"-CPU " + L"-CPUBoundInput " + L"-BGR ";
            Assert::AreEqual(0, RunProc((wchar_t *)command.c_str()));
        }

        TEST_METHOD(GarbageInputCpuDeviceCpuBoundTensor)
        {
            auto const curPath = FileHelper::GetModulePath();
            std::wstring command = curPath +
                L"./WinMLRunner " + L"-model " + curPath + L"SqueezeNet.onnx " + L"-CPU " + L"-CPUBoundInput " + L"-tensor ";
            Assert::AreEqual(0, RunProc((wchar_t *)command.c_str()));
        }

        TEST_METHOD(GarbageInputCpuDeviceGpuBoundRGBImage)
        {
            auto const curPath = FileHelper::GetModulePath();
            std::wstring command = curPath +
                L"./WinMLRunner " + L"-model " + curPath + L"SqueezeNet.onnx " + L"-CPU " + L"-GPUBoundInput " + L"-RGB ";
            Assert::AreEqual(0, RunProc((wchar_t *)command.c_str()));
        }

        TEST_METHOD(GarbageInputCpuDeviceGpuBoundBGRImage)
        {
            auto const curPath = FileHelper::GetModulePath();
            std::wstring command = curPath +
                L"./WinMLRunner " + L"-model " + curPath + L"SqueezeNet.onnx " + L"-CPU " + L"-GPUBoundInput " + L"-BGR ";
            Assert::AreEqual(0, RunProc((wchar_t *)command.c_str()));
        }

        TEST_METHOD(GarbageInputCpuDeviceGpuBoundTensor)
        {
            auto const curPath = FileHelper::GetModulePath();
            std::wstring command = curPath +
                L"./WinMLRunner " + L"-model " + curPath + L"SqueezeNet.onnx " + L"-CPU " + L"-GPUBoundInput " + L"-tensor ";
            Assert::AreEqual(0, RunProc((wchar_t *)command.c_str()));
        }

        TEST_METHOD(GarbageInputGpuDeviceCpuBoundRGBImage)
        {
            auto const curPath = FileHelper::GetModulePath();
            std::wstring command = curPath +
                L"./WinMLRunner " + L"-model " + curPath + L"SqueezeNet.onnx " + L"-GPU " + L"-CPUBoundInput " + L"-RGB ";
            Assert::AreEqual(0, RunProc((wchar_t *)command.c_str()));
        }

        TEST_METHOD(GarbageInputGpuDeviceCpuBoundBGRImage)
        {
            auto const curPath = FileHelper::GetModulePath();
            std::wstring command = curPath +
                L"./WinMLRunner " + L"-model " + curPath + L"SqueezeNet.onnx " + L"-GPU " + L"-CPUBoundInput " + L"-BGR ";
            Assert::AreEqual(0, RunProc((wchar_t *)command.c_str()));
        }

        TEST_METHOD(GarbageInputGpuDeviceCpuBoundTensor)
        {
            auto const curPath = FileHelper::GetModulePath();
            std::wstring command = curPath +
                L"./WinMLRunner " + L"-model " + curPath + L"SqueezeNet.onnx " + L"-GPU " + L"-CPUBoundInput " + L"-tensor ";
            Assert::AreEqual(0, RunProc((wchar_t *)command.c_str()));
        }

        TEST_METHOD(GarbageInputGpuDeviceGpuBoundRGBImage)
        {
            auto const curPath = FileHelper::GetModulePath();
            std::wstring command = curPath +
                L"./WinMLRunner " + L"-model " + curPath + L"SqueezeNet.onnx " + L"-GPU " + L"-GPUBoundInput " + L"-RGB ";
            Assert::AreEqual(0, RunProc((wchar_t *)command.c_str()));
        }

        TEST_METHOD(GarbageInputGpuDeviceGpuBoundBGRImage)
        {
            auto const curPath = FileHelper::GetModulePath();
            std::wstring command = curPath +
                L"./WinMLRunner " + L"-model " + curPath + L"SqueezeNet.onnx " + L"-GPU " + L"-GPUBoundInput " + L"-BGR ";
            Assert::AreEqual(0, RunProc((wchar_t *)command.c_str()));
        }

        TEST_METHOD(GarbageInputGpuDeviceGpuBoundTensor)
        {
            auto const curPath = FileHelper::GetModulePath();
            std::wstring command = curPath +
                L"./WinMLRunner " + L"-model " + curPath + L"SqueezeNet.onnx " + L"-GPU " + L"-GPUBoundInput " + L"-tensor ";
            Assert::AreEqual(0, RunProc((wchar_t *)command.c_str()));
        }

        TEST_METHOD(GarbageInputAllPermutations)
        {
            auto const curPath = FileHelper::GetModulePath();
            std::wstring command = curPath +
                L"./WinMLRunner " + L"-model " + curPath + L"SqueezeNet.onnx " + L"-GPU -CPU -CPUBoundInput -GPUBoundInput -RGB -BGR -tensor";
            Assert::AreEqual(0, RunProc((wchar_t *)command.c_str()));
        }

        TEST_METHOD(RunAllModelsInFolderGarbageInput)
        {
            auto const curPath = FileHelper::GetModulePath();
            auto modelList = { "SqueezeNet.onnx",
                "Add_ImageNet1920.onnx" };

            //make test_models folder
            std::wstring testFolderName = L"test_folder_input";
            std::string mkFolderCommand = "mkdir " + std::string(testFolderName.begin(), testFolderName.end());
            system(mkFolderCommand.c_str());

            //copy models from list to test_folder_input
            for (auto model : modelList)
            {
                std::string copyCommand = "Copy";
                copyCommand += " .\\";
                copyCommand += model;
                copyCommand += " .\\" + std::string(testFolderName.begin(), testFolderName.end());
                system(copyCommand.c_str());
            }

            std::wstring command = curPath +
                L"./WinMLRunner " + L"-folder " + testFolderName;
            Assert::AreEqual(0, RunProc((wchar_t *)command.c_str()));
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

            auto abc = RunProc((wchar_t *)command.c_str());

            Assert::AreNotEqual(0, abc);
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