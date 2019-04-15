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

    bool
    CompareTensorsProvidedEpsilonAndRelativeTolerance(const std::vector<std::pair<int, float>>& expectedOutputTensors,
                                                      const std::vector<std::pair<int, float>>& actualOutputTensors,
                                                      float relativeTolerance, float epsilon)
    {
        if (expectedOutputTensors.size() != 0 && actualOutputTensors.size() != 0 &&
            expectedOutputTensors.size() != actualOutputTensors.size())
        {
            Assert::Fail(
                L"One of the output tensors is empty or expected and Actual Output tensors are different sizes\n");
        }
        bool doesActualMatchExpected = true;
        for (int i = 0; i < expectedOutputTensors.size(); i++)
        {
            float actualValueNum = actualOutputTensors[i].second;
            float expectedValueNum = expectedOutputTensors[i].second;
            if (std::abs(actualValueNum - expectedValueNum) > 0.001 &&
                std::abs(actualValueNum - expectedValueNum) >
                    relativeTolerance * std::abs(expectedValueNum) + epsilon) // Check if the values are too different.
            {
                printf("Expected and Actual tensor value is too different at Index: %d. Expected: %f, Actual: %f\n", i,
                       expectedValueNum, actualValueNum);
                doesActualMatchExpected = false;
            }
        }
        return doesActualMatchExpected;
    }

    void PopulateTensorLists(const std::wstring& tensorFile, std::vector<std::pair<int, float>>& tensorList)
    {
        std::ifstream tensorFileStream;
        tensorFileStream.open(tensorFile);
        std::string index;
        std::string value;
        if (tensorFileStream.fail())
        {
            Assert::Fail(L"Failed to open tensor files\n");
        }
        bool isFirstRow = true;
        while (!tensorFileStream.eof())
        {
            std::getline(tensorFileStream, index, ',');
            std::getline(tensorFileStream, value, '\n');
            if (isFirstRow)
            {
                isFirstRow = false;
                continue;
            }
            if (value != "" && index != "")
            {
                tensorList.push_back(std::make_pair(std::stoi(index), std::stof(value)));
            }
        }
    }

    // This method sorts the expected output tensors and actual output tensors from largest tensor value to smallest
    // tensor value. It takes the percentage decrease between the highest tensor value to the next highest tensor value
    // for both sorted tensor lists and so on. Using relative tolerance and epsilon, we can compare between expected
    // percentage decrease with actual percentage decrease.
    bool CompareTensorValuesRelative(std::vector<std::pair<int, float>>& expectedOutputTensors,
                                     std::vector<std::pair<int, float>>& actualOutputTensors,
                                     const float relativeTolerance, const float epsilon,
                                     const float smallestValueToCompare)
    {
        if (expectedOutputTensors.size() != 0 && actualOutputTensors.size() != 0 &&
            expectedOutputTensors.size() != actualOutputTensors.size())
        {
            Assert::Fail(
                L"One of the output tensors is empty or expected and Actual Output tensors are different sizes\n");
        }
        // Sort expected and actual output tensors from highest to lowest. NOTE: This will modify the original
        // parameters.
        std::sort(expectedOutputTensors.begin(), expectedOutputTensors.end(),
                  [](auto& left, auto& right) { return left.second > right.second; });
        std::sort(actualOutputTensors.begin(), actualOutputTensors.end(),
                  [](auto& left, auto& right) { return left.second > right.second; });

        bool currentValueIsLargeEnough = true;
        bool doesActualMatchExpected = true;
        int currentIndex = 0;
        while (currentValueIsLargeEnough && currentIndex < expectedOutputTensors.size())
        {
            // Compare expected vs actual prediction index
            if (expectedOutputTensors[currentIndex].first != actualOutputTensors[currentIndex].first)
            {
                printf("Top Expected Index:%d and Actual Index:%d don't match!",
                       expectedOutputTensors[currentIndex].first, actualOutputTensors[currentIndex].first);
                doesActualMatchExpected = false;
            }
            else if (currentIndex > 0)
            {
                float expectedTensorRatio =
                    (expectedOutputTensors[currentIndex].second - expectedOutputTensors[currentIndex - 1].second) /
                    expectedOutputTensors[currentIndex - 1].second;
                float actualTensorRatio =
                    (actualOutputTensors[currentIndex].second - actualOutputTensors[currentIndex - 1].second) /
                    actualOutputTensors[currentIndex - 1].second;
                // Compare the percentage difference between top values
                if (std::abs(expectedTensorRatio - actualTensorRatio) >
                    relativeTolerance * std::abs(expectedTensorRatio) + epsilon)
                {
                    printf("Actual ratio difference of top values between index %d and index %d don't match expected "
                           "ratio difference",
                           currentIndex - 1, currentIndex);
                    doesActualMatchExpected = false;
                }
            }
            currentValueIsLargeEnough = expectedOutputTensors[++currentIndex].second > smallestValueToCompare;
        }
        return doesActualMatchExpected;
    }

    bool CompareTensors(const std::wstring& expectedOutputTensorFile, const std::wstring& actualOutputTensorFile)
    {
        std::vector<std::pair<int, float>> expectedOutputTensors;
        std::vector<std::pair<int, float>> actualOutputTensors;
        PopulateTensorLists(expectedOutputTensorFile, expectedOutputTensors);
        PopulateTensorLists(actualOutputTensorFile, actualOutputTensors);
        return CompareTensorsProvidedEpsilonAndRelativeTolerance(expectedOutputTensors, actualOutputTensors, 0.003f, 0);
    }

    bool CompareTensorsFP16(const std::wstring& expectedOutputTensorFile, const std::wstring& actualOutputTensorFile)
    {
        std::vector<std::pair<int, float>> expectedOutputTensors;
        std::vector<std::pair<int, float>> actualOutputTensors;
        PopulateTensorLists(expectedOutputTensorFile, expectedOutputTensors);
        PopulateTensorLists(actualOutputTensorFile, actualOutputTensors);
        bool compareAllTensorsResult =
            CompareTensorsProvidedEpsilonAndRelativeTolerance(expectedOutputTensors, actualOutputTensors, 0.06f, 0);
        if (!compareAllTensorsResult) // fall back to more forgiving comparison that compares order of top indexes
        {
            // After calling CompareTensorValuesRelative, the tensor lists will be sorted from largest to smallest
            return CompareTensorValuesRelative(expectedOutputTensors, actualOutputTensors, 0.1f, 0.05f, 0.001f);
        }
        return true;
    }

    TEST_CLASS(GarbageInputTest) {
public: TEST_CLASS_INITIALIZE(SetupClass) {
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
} // namespace WinMLRunnerTest

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
            // TODO: this will not work if each test runs in parallel. Use different file path for each test, and clean
            // up at class setup
            std::remove(std::string(OUTPUT_PATH.begin(), OUTPUT_PATH.end()).c_str());
            try
            {
                std::filesystem::remove_all(std::string(TENSOR_DATA_PATH.begin(), TENSOR_DATA_PATH.end()).c_str());
            }
            catch (const std::filesystem::filesystem_error&)
            {
            }
        }
        TEST_METHOD(GarbageInputCpuAndGpu)
        {
            const std::wstring modelPath = CURRENT_PATH + L"SqueezeNet.onnx";
            const std::wstring command =
                BuildCommand({ EXE_PATH, L"-model", modelPath, L"-PerfOutput", OUTPUT_PATH, L"-perf" });
            Assert::AreEqual(S_OK, RunProc((wchar_t*)command.c_str()));

            // We need to expect one more line because of the header
            Assert::AreEqual(static_cast<size_t>(3), GetOutputCSVLineCount());
        }

        TEST_METHOD(GarbageInputOnlyCpu)
        {
            const std::wstring modelPath = CURRENT_PATH + L"SqueezeNet.onnx";
            const std::wstring command =
                BuildCommand({ EXE_PATH, L"-model", modelPath, L"-PerfOutput", OUTPUT_PATH, L"-perf", L"-CPU" });
            Assert::AreEqual(S_OK, RunProc((wchar_t*)command.c_str()));

            // We need to expect one more line because of the header
            Assert::AreEqual(static_cast<size_t>(2), GetOutputCSVLineCount());
        }

        TEST_METHOD(GarbageInputOnlyGpu)
        {
            const std::wstring modelPath = CURRENT_PATH + L"SqueezeNet.onnx";
            const std::wstring command =
                BuildCommand({ EXE_PATH, L"-model", modelPath, L"-PerfOutput", OUTPUT_PATH, L"-perf", L"-GPU" });
            Assert::AreEqual(S_OK, RunProc((wchar_t*)command.c_str()));

            // We need to expect one more line because of the header
            Assert::AreEqual(static_cast<size_t>(2), GetOutputCSVLineCount());
        }
        TEST_METHOD(GarbageInputCpuWinMLDeviceCpuBoundRGBImage)
        {
            const std::wstring modelPath = CURRENT_PATH + L"SqueezeNet.onnx";
            const std::wstring command =
                BuildCommand({ EXE_PATH, L"-model", modelPath, L"-PerfOutput", OUTPUT_PATH, L"-perf", L"-CPU",
                               L"-CPUBoundInput", L"-RGB", L"-CreateDeviceInWinML" });
            Assert::AreEqual(S_OK, RunProc((wchar_t*)command.c_str()));

            // We need to expect one more line because of the header
            Assert::AreEqual(static_cast<size_t>(2), GetOutputCSVLineCount());
        }

        TEST_METHOD(GarbageInputCpuWinMLDeviceCpuBoundBGRImage)
        {
            const std::wstring modelPath = CURRENT_PATH + L"SqueezeNet.onnx";
            const std::wstring command =
                BuildCommand({ EXE_PATH, L"-model", modelPath, L"-PerfOutput", OUTPUT_PATH, L"-perf", L"-CPU",
                               L"-CPUBoundInput", L"-BGR", L"-CreateDeviceInWinML" });
            Assert::AreEqual(S_OK, RunProc((wchar_t*)command.c_str()));

            // We need to expect one more line because of the header
            Assert::AreEqual(static_cast<size_t>(2), GetOutputCSVLineCount());
        }

        TEST_METHOD(GarbageInputCpuWinMLDeviceCpuBoundTensor)
        {
            const std::wstring modelPath = CURRENT_PATH + L"SqueezeNet.onnx";
            const std::wstring command =
                BuildCommand({ EXE_PATH, L"-model", modelPath, L"-PerfOutput", OUTPUT_PATH, L"-perf", L"-CPU",
                               L"-CPUBoundInput", L"-tensor", L"-CreateDeviceInWinML" });
            Assert::AreEqual(S_OK, RunProc((wchar_t*)command.c_str()));

            // We need to expect one more line because of the header
            Assert::AreEqual(static_cast<size_t>(2), GetOutputCSVLineCount());
        }
        TEST_METHOD(GarbageInputCpuWinMLDeviceGpuBoundRGBImage)
        {
            const std::wstring modelPath = CURRENT_PATH + L"SqueezeNet.onnx";
            const std::wstring command =
                BuildCommand({ EXE_PATH, L"-model", modelPath, L"-PerfOutput", OUTPUT_PATH, L"-perf", L"-CPU",
                               L"-GPUBoundInput", L"-RGB", L"-CreateDeviceInWinML" });
            Assert::AreEqual(S_OK, RunProc((wchar_t*)command.c_str()));

            // We need to expect one more line because of the header
            Assert::AreEqual(static_cast<size_t>(2), GetOutputCSVLineCount());
        }

        TEST_METHOD(GarbageInputCpuWinMLDeviceGpuBoundBGRImage)
        {
            const std::wstring modelPath = CURRENT_PATH + L"SqueezeNet.onnx";
            const std::wstring command =
                BuildCommand({ EXE_PATH, L"-model", modelPath, L"-PerfOutput", OUTPUT_PATH, L"-perf", L"-CPU",
                               L"-GPUBoundInput", L"-BGR", L"-CreateDeviceInWinML" });
            Assert::AreEqual(S_OK, RunProc((wchar_t*)command.c_str()));

            // We need to expect one more line because of the header
            Assert::AreEqual(static_cast<size_t>(2), GetOutputCSVLineCount());
        }
        TEST_METHOD(GarbageInputCpuWinMLDeviceGpuBoundTensor)
        {
            const std::wstring modelPath = CURRENT_PATH + L"SqueezeNet.onnx";
            const std::wstring command =
                BuildCommand({ EXE_PATH, L"-model", modelPath, L"-PerfOutput", OUTPUT_PATH, L"-perf", L"-CPU",
                               L"-GPUBoundInput", L"-tensor", L"-CreateDeviceInWinML" });
            // Binding GPU Tensor with Session created with CPU device isn't supported.
            Assert::AreEqual(E_INVALIDARG, RunProc((wchar_t*)command.c_str()));
        }
        TEST_METHOD(GarbageInputGpuClientDeviceCpuBoundRGBImage)
        {
            const std::wstring modelPath = CURRENT_PATH + L"SqueezeNet.onnx";
            const std::wstring command =
                BuildCommand({ EXE_PATH, L"-model", modelPath, L"-PerfOutput", OUTPUT_PATH, L"-perf", L"-GPU",
                               L"-CPUBoundInput", L"-RGB", L"-CreateDeviceOnClient" });
            Assert::AreEqual(S_OK, RunProc((wchar_t*)command.c_str()));

            // We need to expect one more line because of the header
            Assert::AreEqual(static_cast<size_t>(2), GetOutputCSVLineCount());
        }

        TEST_METHOD(GarbageInputGpuWinMLDeviceCpuBoundRGBImage)
        {
            const std::wstring modelPath = CURRENT_PATH + L"SqueezeNet.onnx";
            const std::wstring command =
                BuildCommand({ EXE_PATH, L"-model", modelPath, L"-PerfOutput", OUTPUT_PATH, L"-perf", L"-GPU",
                               L"-CPUBoundInput", L"-RGB", L"-CreateDeviceInWinML" });
            Assert::AreEqual(S_OK, RunProc((wchar_t*)command.c_str()));

            // We need to expect one more line because of the header
            Assert::AreEqual(static_cast<size_t>(2), GetOutputCSVLineCount());
        }

        TEST_METHOD(GarbageInputGpuClientDeviceCpuBoundBGRImage)
        {
            const std::wstring modelPath = CURRENT_PATH + L"SqueezeNet.onnx";
            const std::wstring command =
                BuildCommand({ EXE_PATH, L"-model", modelPath, L"-PerfOutput", OUTPUT_PATH, L"-perf", L"-GPU",
                               L"-CPUBoundInput", L"-BGR", L"-CreateDeviceOnClient" });
            Assert::AreEqual(S_OK, RunProc((wchar_t*)command.c_str()));

            // We need to expect one more line because of the header
            Assert::AreEqual(static_cast<size_t>(2), GetOutputCSVLineCount());
        }

        TEST_METHOD(GarbageInputGpuWinMLDeviceCpuBoundBGRImage)
        {
            const std::wstring modelPath = CURRENT_PATH + L"SqueezeNet.onnx";
            const std::wstring command =
                BuildCommand({ EXE_PATH, L"-model", modelPath, L"-PerfOutput", OUTPUT_PATH, L"-perf", L"-GPU",
                               L"-CPUBoundInput", L"-BGR", L"-CreateDeviceInWinML" });
            Assert::AreEqual(S_OK, RunProc((wchar_t*)command.c_str()));

            // We need to expect one more line because of the header
            Assert::AreEqual(static_cast<size_t>(2), GetOutputCSVLineCount());
        }

        TEST_METHOD(GarbageInputGpuClientDeviceCpuBoundTensor)
        {
            const std::wstring modelPath = CURRENT_PATH + L"SqueezeNet.onnx";
            const std::wstring command =
                BuildCommand({ EXE_PATH, L"-model", modelPath, L"-PerfOutput", OUTPUT_PATH, L"-perf", L"-GPU",
                               L"-CPUBoundInput", L"-tensor", L"-CreateDeviceOnClient" });
            Assert::AreEqual(S_OK, RunProc((wchar_t*)command.c_str()));

            // We need to expect one more line because of the header
            Assert::AreEqual(static_cast<size_t>(2), GetOutputCSVLineCount());
        }

        TEST_METHOD(GarbageInputGpuWinMLDeviceCpuBoundTensor)
        {
            const std::wstring modelPath = CURRENT_PATH + L"SqueezeNet.onnx";
            const std::wstring command =
                BuildCommand({ EXE_PATH, L"-model", modelPath, L"-PerfOutput", OUTPUT_PATH, L"-perf", L"-GPU",
                               L"-CPUBoundInput", L"-tensor", L"-CreateDeviceInWinML" });
            Assert::AreEqual(S_OK, RunProc((wchar_t*)command.c_str()));

            // We need to expect one more line because of the header
            Assert::AreEqual(static_cast<size_t>(2), GetOutputCSVLineCount());
        }

        TEST_METHOD(GarbageInputGpuClientDeviceGpuBoundRGBImage)
        {
            const std::wstring modelPath = CURRENT_PATH + L"SqueezeNet.onnx";
            const std::wstring command =
                BuildCommand({ EXE_PATH, L"-model", modelPath, L"-PerfOutput", OUTPUT_PATH, L"-perf", L"-GPU",
                               L"-GPUBoundInput", L"-RGB", L"-CreateDeviceOnClient" });
            Assert::AreEqual(S_OK, RunProc((wchar_t*)command.c_str()));

            // We need to expect one more line because of the header
            Assert::AreEqual(static_cast<size_t>(2), GetOutputCSVLineCount());
        }

        TEST_METHOD(GarbageInputGpuWinMLDeviceGpuBoundRGBImage)
        {
            const std::wstring modelPath = CURRENT_PATH + L"SqueezeNet.onnx";
            const std::wstring command =
                BuildCommand({ EXE_PATH, L"-model", modelPath, L"-PerfOutput", OUTPUT_PATH, L"-perf", L"-GPU",
                               L"-GPUBoundInput", L"-RGB", L"-CreateDeviceInWinML" });
            Assert::AreEqual(S_OK, RunProc((wchar_t*)command.c_str()));

            // We need to expect one more line because of the header
            Assert::AreEqual(static_cast<size_t>(2), GetOutputCSVLineCount());
        }

        TEST_METHOD(GarbageInputGpuClientDeviceGpuBoundBGRImage)
        {
            const std::wstring modelPath = CURRENT_PATH + L"SqueezeNet.onnx";
            const std::wstring command =
                BuildCommand({ EXE_PATH, L"-model", modelPath, L"-PerfOutput", OUTPUT_PATH, L"-perf", L"-GPU",
                               L"-GPUBoundInput", L"-BGR", L"-CreateDeviceOnClient" });
            Assert::AreEqual(S_OK, RunProc((wchar_t*)command.c_str()));

            // We need to expect one more line because of the header
            Assert::AreEqual(static_cast<size_t>(2), GetOutputCSVLineCount());
        }

        TEST_METHOD(GarbageInputGpuWinMLDeviceGpuBoundBGRImage)
        {
            const std::wstring modelPath = CURRENT_PATH + L"SqueezeNet.onnx";
            const std::wstring command =
                BuildCommand({ EXE_PATH, L"-model", modelPath, L"-PerfOutput", OUTPUT_PATH, L"-perf", L"-GPU",
                               L"-GPUBoundInput", L"-BGR", L"-CreateDeviceInWinML" });
            Assert::AreEqual(S_OK, RunProc((wchar_t*)command.c_str()));

            // We need to expect one more line because of the header
            Assert::AreEqual(static_cast<size_t>(2), GetOutputCSVLineCount());
        }

        TEST_METHOD(GarbageInputGpuClientDeviceGpuBoundTensor)
        {
            const std::wstring modelPath = CURRENT_PATH + L"SqueezeNet.onnx";
            const std::wstring command =
                BuildCommand({ EXE_PATH, L"-model", modelPath, L"-PerfOutput", OUTPUT_PATH, L"-perf", L"-GPU",
                               L"-GPUBoundInput", L"-tensor", L"-CreateDeviceOnClient" });
            Assert::AreEqual(S_OK, RunProc((wchar_t*)command.c_str()));

            // We need to expect one more line because of the header
            Assert::AreEqual(static_cast<size_t>(2), GetOutputCSVLineCount());
        }

        TEST_METHOD(GarbageInputGpuWinMLDeviceGpuBoundTensor)
        {
            const std::wstring modelPath = CURRENT_PATH + L"SqueezeNet.onnx";
            const std::wstring command =
                BuildCommand({ EXE_PATH, L"-model", modelPath, L"-PerfOutput", OUTPUT_PATH, L"-perf", L"-GPU",
                               L"-GPUBoundInput", L"-tensor", L"-CreateDeviceInWinML" });
            Assert::AreEqual(S_OK, RunProc((wchar_t*)command.c_str()));

            // We need to expect one more line because of the header
            Assert::AreEqual(static_cast<size_t>(2), GetOutputCSVLineCount());
        }

        TEST_METHOD(RunAllModelsInFolderGarbageInput)
        {
            const std::wstring command = BuildCommand({ EXE_PATH, L"-folder", INPUT_FOLDER_PATH, L"-PerfOutput", OUTPUT_PATH, L"-perf" });
            Assert::AreEqual(S_OK, RunProc((wchar_t *)command.c_str()));

            // We need to expect one more line because of the header
            Assert::AreEqual(static_cast<size_t>(5), GetOutputCSVLineCount());
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
            const std::wstring command =
                BuildCommand({ EXE_PATH, L"-model ", modelPath, L"-input", inputPath, L"-autoScale", L"Cubic" });
            Assert::AreEqual(S_OK, RunProc((wchar_t*)command.c_str()));
        }
        TEST_METHOD(ProvidedImageInputOnlyGpuSaveTensor)
        {
            const std::wstring modelPath = CURRENT_PATH + L"SqueezeNet.onnx";
            const std::wstring inputPath = CURRENT_PATH + L"fish.png";
            const std::wstring command = BuildCommand({ EXE_PATH, L"-model ", modelPath, L"-input", inputPath,
                                                        L"-SaveTensorData", L"First", TENSOR_DATA_PATH, L"-GPU" });
            Assert::AreEqual(S_OK, RunProc((wchar_t*)command.c_str()));
            Assert::AreEqual(true, CompareTensors(L"OutputTensorData\\Squeezenet_fish_input_GPU.csv",
                                                  TENSOR_DATA_PATH + L"\\softmaxout_1GpuIteration1.csv"));
        }
        TEST_METHOD(ProvidedImageInputOnlyCpuSaveTensor)
        {
            const std::wstring modelPath = CURRENT_PATH + L"SqueezeNet.onnx";
            const std::wstring inputPath = CURRENT_PATH + L"fish.png";
            const std::wstring command = BuildCommand({ EXE_PATH, L"-model ", modelPath, L"-input", inputPath,
                                                        L"-SaveTensorData", L"First", TENSOR_DATA_PATH, L"-CPU" });
            Assert::AreEqual(S_OK, RunProc((wchar_t*)command.c_str()));
            Assert::AreEqual(true, CompareTensors(L"OutputTensorData\\Squeezenet_fish_input_CPU.csv",
                                                  TENSOR_DATA_PATH + L"\\softmaxout_1CpuIteration1.csv"));
        }

        TEST_METHOD(ProvidedImageInputOnlyGpuSaveTensorImageDenotation)
        {
            const std::wstring modelPath = CURRENT_PATH + L"mnist.onnx";
            const std::wstring inputPath = CURRENT_PATH + L"mnist_28.png";
            const std::wstring command = BuildCommand({ EXE_PATH, L"-model ", modelPath, L"-input", inputPath,
                                                        L"-SaveTensorData", L"First", TENSOR_DATA_PATH, L"-GPU" });
            Assert::AreEqual(S_OK, RunProc((wchar_t*)command.c_str()));
            Assert::AreEqual(true, CompareTensors(L"OutputTensorData\\Mnist_8_input_GPU.csv",
                TENSOR_DATA_PATH + L"\\Plus214_Output_0GpuIteration1.csv"));
        }
        TEST_METHOD(ProvidedImageInputOnlyCpuSaveTensorImageDenotation)
        {
            const std::wstring modelPath = CURRENT_PATH + L"mnist.onnx";
            const std::wstring inputPath = CURRENT_PATH + L"mnist_28.png";
            const std::wstring command = BuildCommand({ EXE_PATH, L"-model ", modelPath, L"-input", inputPath,
                                                        L"-SaveTensorData", L"First", TENSOR_DATA_PATH, L"-CPU" });
            Assert::AreEqual(S_OK, RunProc((wchar_t*)command.c_str()));
            Assert::AreEqual(true, CompareTensors(L"OutputTensorData\\Mnist_8_input_CPU.csv",
                TENSOR_DATA_PATH + L"\\Plus214_Output_0CpuIteration1.csv"));
        }
        TEST_METHOD(ProvidedImageInputOnlyGpuSaveTensorFp16)
        {
            const std::wstring modelPath = CURRENT_PATH + L"SqueezeNet_fp16.onnx";
            const std::wstring inputPath = CURRENT_PATH + L"fish.png";
            const std::wstring command = BuildCommand({ EXE_PATH, L"-model ", modelPath, L"-input", inputPath,
                                                        L"-SaveTensorData", L"First", TENSOR_DATA_PATH, L"-GPU" });
            Assert::AreEqual(S_OK, RunProc((wchar_t*)command.c_str()));
            Assert::AreEqual(true, CompareTensorsFP16(L"OutputTensorData\\Squeezenet_fp16_fish_input_GPU.csv",
                                                      TENSOR_DATA_PATH + L"\\softmaxout_1GpuIteration1.csv"));
        }
        TEST_METHOD(ProvidedImageInputOnlyCpuSaveTensorFp16)
        {
            const std::wstring modelPath = CURRENT_PATH + L"SqueezeNet_fp16.onnx";
            const std::wstring inputPath = CURRENT_PATH + L"fish.png";
            const std::wstring command = BuildCommand({ EXE_PATH, L"-model ", modelPath, L"-input", inputPath,
                                                        L"-SaveTensorData", L"First", TENSOR_DATA_PATH, L"-CPU" });
            Assert::AreEqual(S_OK, RunProc((wchar_t*)command.c_str()));
            Assert::AreEqual(true, CompareTensorsFP16(L"OutputTensorData\\Squeezenet_fp16_fish_input_CPU.csv",
                                                      TENSOR_DATA_PATH + L"\\softmaxout_1CpuIteration1.csv"));
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
        TEST_METHOD(ProvidedCSVInputGPUSaveCpuBoundTensor)
        {
            const std::wstring modelPath = CURRENT_PATH + L"SqueezeNet.onnx";
            const std::wstring inputPath = CURRENT_PATH + L"fish.csv";
            const std::wstring command = BuildCommand({ EXE_PATH, L"-model", modelPath, L"-input", inputPath,
                                                        L"-SaveTensorData", L"First", TENSOR_DATA_PATH, L"-GPU" });
            Assert::AreEqual(S_OK, RunProc((wchar_t*)command.c_str()));
            Assert::AreEqual(true, CompareTensors(L"OutputTensorData\\Squeezenet_fish_input_GPU.csv",
                                                  TENSOR_DATA_PATH + L"\\softmaxout_1GpuIteration1.csv"));
        }
        TEST_METHOD(ProvidedCSVInputGPUSaveGpuBoundTensor)
        {
            const std::wstring modelPath = CURRENT_PATH + L"SqueezeNet.onnx";
            const std::wstring inputPath = CURRENT_PATH + L"fish.csv";
            const std::wstring command = BuildCommand({ EXE_PATH, L"-model", modelPath, L"-input", inputPath,
                                                        L"-SaveTensorData", L"First", TENSOR_DATA_PATH, L"-GPU", L"-GPUBoundInput" });
            Assert::AreEqual(S_OK, RunProc((wchar_t*)command.c_str()));
            Assert::AreEqual(true, CompareTensors(L"OutputTensorData\\Squeezenet_fish_input_GPU.csv",
                TENSOR_DATA_PATH + L"\\softmaxout_1GpuIteration1.csv"));
        }
        TEST_METHOD(ProvidedCSVInputCPUSaveCpuBoundTensor)
        {
            const std::wstring modelPath = CURRENT_PATH + L"SqueezeNet.onnx";
            const std::wstring inputPath = CURRENT_PATH + L"fish.csv";
            const std::wstring command = BuildCommand({ EXE_PATH, L"-model", modelPath, L"-input", inputPath,
                                                        L"-SaveTensorData", L"First", TENSOR_DATA_PATH, L"-CPU" });
            Assert::AreEqual(S_OK, RunProc((wchar_t*)command.c_str()));
            Assert::AreEqual(true, CompareTensors(L"OutputTensorData\\Squeezenet_fish_input_CPU.csv",
                                                  TENSOR_DATA_PATH + L"\\softmaxout_1CpuIteration1.csv"));
        }
        TEST_METHOD(ProvidedCSVInputGPUSaveCpuBoundTensorFp16)
        {
            const std::wstring modelPath = CURRENT_PATH + L"SqueezeNet_fp16.onnx";
            const std::wstring inputPath = CURRENT_PATH + L"fish.csv";
            const std::wstring command = BuildCommand({ EXE_PATH, L"-model", modelPath, L"-input", inputPath,
                                                        L"-SaveTensorData", L"First", TENSOR_DATA_PATH, L"-GPU" });
            Assert::AreEqual(S_OK, RunProc((wchar_t*)command.c_str()));
            Assert::AreEqual(true, CompareTensorsFP16(L"OutputTensorData\\Squeezenet_fp16_fish_input_GPU.csv",
                                                      TENSOR_DATA_PATH + L"\\softmaxout_1GpuIteration1.csv"));
        }
        TEST_METHOD(ProvidedCSVInputCPUSaveCpuBoundTensorFp16)
        {
            const std::wstring modelPath = CURRENT_PATH + L"SqueezeNet_fp16.onnx";
            const std::wstring inputPath = CURRENT_PATH + L"fish.csv";
            const std::wstring command = BuildCommand({ EXE_PATH, L"-model", modelPath, L"-input", inputPath,
                                                        L"-SaveTensorData", L"First", TENSOR_DATA_PATH, L"-CPU" });
            Assert::AreEqual(S_OK, RunProc((wchar_t*)command.c_str()));
            Assert::AreEqual(true, CompareTensorsFP16(L"OutputTensorData\\Squeezenet_fp16_fish_input_CPU.csv",
                                                      TENSOR_DATA_PATH + L"\\softmaxout_1CpuIteration1.csv"));
        }

        TEST_METHOD(ProvidedCSVInputOnlyGpuSaveCpuBoundTensorImageDenotation)
        {
            const std::wstring modelPath = CURRENT_PATH + L"mnist.onnx";
            const std::wstring inputPath = CURRENT_PATH + L"mnist_28.csv";
            const std::wstring command = BuildCommand({ EXE_PATH, L"-model ", modelPath, L"-input", inputPath,
                                                        L"-SaveTensorData", L"First", TENSOR_DATA_PATH, L"-GPU" });
            Assert::AreEqual(S_OK, RunProc((wchar_t*)command.c_str()));
            Assert::AreEqual(true, CompareTensors(L"OutputTensorData\\Mnist_8_input_GPU.csv",
                TENSOR_DATA_PATH + L"\\Plus214_Output_0GpuIteration1.csv"));
        }
        TEST_METHOD(ProvidedCSVInputOnlyCpuSaveCpuBoundTensorImageDenotation)
        {
            const std::wstring modelPath = CURRENT_PATH + L"mnist.onnx";
            const std::wstring inputPath = CURRENT_PATH + L"mnist_28.csv";
            const std::wstring command = BuildCommand({ EXE_PATH, L"-model ", modelPath, L"-input", inputPath,
                                                        L"-SaveTensorData", L"First", TENSOR_DATA_PATH, L"-CPU" });
            Assert::AreEqual(S_OK, RunProc((wchar_t*)command.c_str()));
            Assert::AreEqual(true, CompareTensors(L"OutputTensorData\\Mnist_8_input_CPU.csv",
                TENSOR_DATA_PATH + L"\\Plus214_Output_0CpuIteration1.csv"));
        }
    };

    TEST_CLASS(ConcurrencyTest)
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

        TEST_METHOD(RunFolder)
        {
            const std::wstring command = BuildCommand({
                EXE_PATH, L"-folder", INPUT_FOLDER_PATH, L"-ConcurrentLoad", L"-NumThreads", L"5"
            });
            Assert::AreEqual(S_OK, RunProc((wchar_t *)command.c_str()));
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
            Assert::AreEqual(S_OK, RunProc((wchar_t*)command.c_str()));
        }
        
        TEST_METHOD(TestTopK)
        {
            const std::wstring command = BuildCommand({ EXE_PATH, L"-model", L"SqueezeNet.onnx", L"-TopK", L"5" });
            Assert::AreEqual(S_OK, RunProc((wchar_t*)command.c_str()));
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
