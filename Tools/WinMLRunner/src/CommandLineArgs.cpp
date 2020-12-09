#include <Windows.h>
#include <string>
#include <iostream>
#include "CommandLineArgs.h"
#include <ctime>
#include <iomanip>
#include <filesystem>
#include <codecvt>
#include "Filehelper.h"
#include "debugapi.h"

void CommandLineArgs::PrintUsage()
{
#ifdef USE_WINML_NUGET
    std::cout << "MicrosoftML Runner" << std::endl;
#else
    std::cout << "WinML Runner" << std::endl;
#endif
    std::cout << " ---------------------------------------------------------------" << std::endl;
#ifdef USE_WINML_NUGET
    std::cout << "MicrosoftMLRunner.exe <-model | -folder> <fully qualified path> [options]" << std::endl;
#else
    std::cout << "WinMLRunner.exe <-model | -folder> <fully qualified path> [options]" << std::endl;
#endif
    std::cout << std::endl;
    std::cout << "options: " << std::endl;
#ifdef USE_WINML_NUGET
    std::cout << "  -version: prints the version information for this build of MicrosoftMLRunner.exe" << std::endl;
#else
    std::cout << "  -version: prints the version information for this build of WinMLRunner.exe" << std::endl;
#endif
    std::cout << "  -CPU : run model on default CPU" << std::endl;
    std::cout << "  -GPU : run model on default GPU" << std::endl;
    std::cout << "  -GPUHighPerformance : run model on GPU with highest performance" << std::endl;
    std::cout << "  -GPUMinPower : run model on GPU with the least power" << std::endl;
#ifdef DXCORE_SUPPORTED_BUILD
    std::cout << "  -GPUAdapterName <adapter name substring>: run model on GPU specified by its name. NOTE: Please "
                 "only use this flag on DXCore supported machines."
              << std::endl;
#endif
    std::cout << "  -CreateDeviceOnClient : create the D3D device on the client and pass it to WinML to create session"
              << std::endl;
    std::cout << "  -CreateDeviceInWinML : create the device inside WinML" << std::endl;
    std::cout << "  -CPUBoundInput : bind the input to the CPU" << std::endl;
    std::cout << "  -GPUBoundInput : bind the input to the GPU" << std::endl;
    std::cout << "  -RGB : load the input as an RGB image" << std::endl;
    std::cout << "  -BGR : load the input as a BGR image" << std::endl;
    std::cout << "  -Tensor [function] : load the input as a tensor, with optional function for input preprocessing"
              << std::endl;
    std::cout << "      Optional function arguments:" << std::endl;
    std::cout << "          Identity(default) : No input transformations will be performed." << std::endl;
    std::cout << "          Normalize <scale> <means> <stddevs> : float scale factor and comma separated per channel "
                 "means and stddev for normalization."
              << std::endl;
    std::cout << "  -Perf [all]: capture performance measurements such as timing and memory usage. Specifying \"all\" "
                 "will output all measurements"
              << std::endl;
    std::cout << "  -Iterations : # times perf measurements will be run/averaged. (maximum: 1024 times)" << std::endl;
    std::cout << "  -Input <path to input file>: binds image or CSV to model" << std::endl;
    std::cout << "  -InputImageFolder <path to directory of images> : specify folder of images to bind to model"
              << std::endl;
    std::cout << "  -TopK <number> : print top <number> values in the result. Default to 1" << std::endl;
    std::cout << "  -GarbageDataMaxValue <number> : limit garbage data range to a max random value" << std::endl;
    std::cout << "  -BaseOutputPath [<fully qualified path>] : base output directory path for results, default to cwd"
              << std::endl;
    std::cout << "  -PerfOutput [<path>] : fully qualified or relative path including csv filename for perf results"
              << std::endl;
    std::cout << "  -SavePerIterationPerf : save per iteration performance results to csv file" << std::endl;
    std::cout << "  -PerIterationPath <directory_path> : Relative or fully qualified path for per iteration and save "
                 "tensor output results.  If not specified a default(timestamped) folder will be created."
              << std::endl;
    std::cout << "  -SaveTensorData <saveMode>: saveMode: save first iteration or all iteration output "
                 "tensor results to csv file [First, All]"
              << std::endl;
    std::cout << "  -DebugEvaluate: Print evaluation debug output to debug console if debugger is present."
              << std::endl;
    std::cout << "  -Terse: Terse Mode (suppresses repetitive console output)" << std::endl;
    std::cout << "  -AutoScale <interpolationMode> : Enable image autoscaling and set the interpolation mode [Nearest, "
                 "Linear, Cubic, Fant]"
              << std::endl;
    std::cout << std::endl;
    std::cout << "Concurrency Options:" << std::endl;
    std::cout << "  -ConcurrentLoad: load models concurrently" << std::endl;
    std::cout << "  -NumThreads <number>: number of threads to load a model. By default this will be the number of "
                 "model files to be executed"
              << std::endl;
    std::cout << "  -ThreadInterval <milliseconds>: interval time between two thread creations in milliseconds"
              << std::endl;
}

void CheckAPICall(int return_value)
{
    if (return_value == 0)
    {
        auto code = GetLastError();
        std::wstring msg = L"failed to get the version of this file with error code: ";
        msg += std::to_wstring(code);
        throw hresult_invalid_argument(msg.c_str());
    }
}

#pragma warning(push)
#pragma warning(disable : 4996)

CommandLineArgs::CommandLineArgs(const std::vector<std::wstring>& args)
{
    std::wstring sPerfOutputPath;
    std::wstring sBaseOutputPath;
    std::wstring sPerIterationDataPath;

    for (UINT i = 0; i < args.size(); i++)
    {
        if ((_wcsicmp(args[i].c_str(), L"-CPU") == 0))
        {
            m_useCPU = true;
        }
        else if ((_wcsicmp(args[i].c_str(), L"-GPU") == 0))
        {
            m_useGPU = true;
        }
        else if ((_wcsicmp(args[i].c_str(), L"-GPUHighPerformance") == 0))
        {
            m_useGPUHighPerformance = true;
        }
        else if ((_wcsicmp(args[i].c_str(), L"-GPUMinPower") == 0))
        {
            m_useGPUMinPower = true;
        }
#ifdef DXCORE_SUPPORTED_BUILD
        else if (_wcsicmp(args[i].c_str(), L"-GPUAdapterName") == 0)
        {
            CheckNextArgument(args, i);
            HMODULE library = nullptr;
            library = LoadLibrary(L"dxcore.dll");
            if (!library)
            {
                throw hresult_invalid_argument(
                    L"ERROR: DXCORE isn't supported on this machine. "
                    L"GpuAdapterName flag should only be used with DXCore supported machines.");
            }
            m_adapterName = args[++i];
            m_useGPU = true;
        }
#endif
        else if ((_wcsicmp(args[i].c_str(), L"-CreateDeviceOnClient") == 0))
        {
            m_createDeviceOnClient = true;
        }
        else if ((_wcsicmp(args[i].c_str(), L"-CreateDeviceInWinML") == 0))
        {
            m_createDeviceInWinML = true;
        }
        else if ((_wcsicmp(args[i].c_str(), L"-Iterations") == 0) && (i + 1 < args.size()))
        {
            m_numIterations = static_cast<UINT>(_wtoi(args[++i].c_str()));
        }
        else if ((_wcsicmp(args[i].c_str(), L"-Model") == 0))
        {
            CheckNextArgument(args, i);
            m_modelPath = FileHelper::GetAbsolutePath(args[++i]);
        }
        else if ((_wcsicmp(args[i].c_str(), L"-Folder") == 0))
        {
            CheckNextArgument(args, i);
            m_modelFolderPath = args[++i];
        }
        else if ((_wcsicmp(args[i].c_str(), L"-Input") == 0))
        {
            CheckNextArgument(args, i);
            m_inputData = FileHelper::GetAbsolutePath(args[++i]);
        }
        else if ((_wcsicmp(args[i].c_str(), L"-InputImageFolder") == 0))
        {
            CheckNextArgument(args, i);
            m_inputImageFolderPath = FileHelper::GetAbsolutePath(args[++i]);
        }
        else if ((_wcsicmp(args[i].c_str(), L"-PerfOutput") == 0))
        {
            if (i + 1 < args.size() && args[i + 1][0] != L'-')
            {
                sPerfOutputPath = args[++i];
            }
            m_perfOutput = true;
        }
        else if ((_wcsicmp(args[i].c_str(), L"-RGB") == 0))
        {
            m_useRGB = true;
        }
        else if ((_wcsicmp(args[i].c_str(), L"-BGR") == 0))
        {
            m_useBGR = true;
        }
        else if (_wcsicmp(args[i].c_str(), L"-Tensor") == 0)
        {
            m_useTensor = true;
            m_tensorizeArgs.Func = TensorizeFuncs::Identity;
            if (i + 1 < args.size() && args[i + 1][0] != L'-')
            {
                if (_wcsicmp(args[++i].c_str(), L"Identity") == 0)
                {
                }
                else if (_wcsicmp(args[i].c_str(), L"Normalize") == 0)
                {
                    CheckNextArgument(args, i, i + 1);
                    CheckNextArgument(args, i, i + 2);
                    CheckNextArgument(args, i, i + 3);

                    m_tensorizeArgs.Func = TensorizeFuncs::Normalize;
                    m_tensorizeArgs.Normalize.Scale = (float)_wtof(args[++i].c_str());

                    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
                    std::istringstream means(converter.to_bytes(args[++i]));
                    std::string mean;
                    while (std::getline(means, mean, ','))
                        m_tensorizeArgs.Normalize.Means.push_back((float)std::stof(mean.c_str()));

                    std::istringstream stddevs(converter.to_bytes(args[++i]));
                    std::string stddev;
                    while (std::getline(stddevs, stddev, ','))
                        m_tensorizeArgs.Normalize.StdDevs.push_back((float)std::stof(stddev.c_str()));

                    if (m_tensorizeArgs.Normalize.Means.size() != m_tensorizeArgs.Normalize.StdDevs.size())
                        throw hresult_invalid_argument(
                            L"-Tensor Normalize: must be the same number of mean and stddev arguments!");
                }
                else
                {
                    std::wstring msg = L"-Tensor unknown option ";
                    msg += args[i].c_str();
                    throw hresult_invalid_argument(msg.c_str());
                }
            }
        }
        else if ((_wcsicmp(args[i].c_str(), L"-CPUBoundInput") == 0))
        {
            m_useCPUBoundInput = true;
        }
        else if ((_wcsicmp(args[i].c_str(), L"-GPUBoundInput") == 0))
        {
            m_useGPUBoundInput = true;
        }
        else if ((_wcsicmp(args[i].c_str(), L"-Perf") == 0))
        {
            if (i + 1 < args.size() && args[i + 1][0] != L'-' && (_wcsicmp(args[i + 1].c_str(), L"all") == 0))
            {
                m_perfConsoleOutputAll = true;
                i++;
            }
            m_perfCapture = true;
        }
        else if ((_wcsicmp(args[i].c_str(), L"-DebugEvaluate") == 0))
        {
            if (!IsDebuggerPresent())
            {
                throw hresult_invalid_argument(
#ifdef USE_WINML_NUGET
                    L"-DebugEvaluate flag should only be used when MicrosoftMLRunner is under a user-mode debugger!"
#else
                    L"-DebugEvaluate flag should only be used when WinMLRunner is under a user-mode debugger!"
#endif
                );
            }
            ToggleEvaluationDebugOutput(true);
        }
        else if ((_wcsicmp(args[i].c_str(), L"-SavePerIterationPerf") == 0))
        {
            m_perIterCapture = true;
        }
        else if (_wcsicmp(args[i].c_str(), L"-BaseOutputPath") == 0)
        {
            CheckNextArgument(args, i);
            sBaseOutputPath = args[++i].c_str();
        }
        else if (_wcsicmp(args[i].c_str(), L"-PerIterationPath") == 0)
        {
            CheckNextArgument(args, i);
            sPerIterationDataPath = args[++i].c_str();
        }
        else if ((_wcsicmp(args[i].c_str(), L"-Terse") == 0))
        {
            m_terseOutput = true;
        }
        else if ((_wcsicmp(args[i].c_str(), L"-AutoScale") == 0))
        {
            CheckNextArgument(args, i);
            m_autoScale = true;
            if (_wcsicmp(args[++i].c_str(), L"Nearest") == 0)
            {
                m_autoScaleInterpMode = BitmapInterpolationMode::NearestNeighbor;
            }
            else if (_wcsicmp(args[i].c_str(), L"Linear") == 0)
            {
                m_autoScaleInterpMode = BitmapInterpolationMode::Linear;
            }
            else if (_wcsicmp(args[i].c_str(), L"Cubic") == 0)
            {
                m_autoScaleInterpMode = BitmapInterpolationMode::Cubic;
            }
            else if (_wcsicmp(args[i].c_str(), L"Fant") == 0)
            {
                m_autoScaleInterpMode = BitmapInterpolationMode::Fant;
            }
            else
            {
                PrintUsage();
                throw hresult_invalid_argument(L"Unknown AutoScale Interpolation Mode!");
            }
        }
        else if (_wcsicmp(args[i].c_str(), L"-SaveTensorData") == 0)
        {
            CheckNextArgument(args, i);
            m_saveTensor = true;
            if (_wcsicmp(args[++i].c_str(), L"First") == 0)
            {
                m_saveTensorMode = L"First";
            }
            else if (_wcsicmp(args[i].c_str(), L"All") == 0)
            {
                m_saveTensorMode = L"All";
            }
            else
            {
                PrintUsage();
                throw hresult_invalid_argument(L"Unknown SaveTensorData Mode[" + m_saveTensorMode + L"]!");
            }
        }
        else if (_wcsicmp(args[i].c_str(), L"-Version") == 0)
        {
            TCHAR szExeFileName[MAX_PATH];
            auto ret = GetModuleFileName(NULL, szExeFileName, MAX_PATH);
            CheckAPICall(ret);
            uint32_t versionInfoSize = GetFileVersionInfoSize(szExeFileName, 0);
            wchar_t* pVersionData = new wchar_t[versionInfoSize / sizeof(wchar_t)];
            CheckAPICall(GetFileVersionInfo(szExeFileName, 0, versionInfoSize, pVersionData));

            wchar_t* pOriginalFilename;
            uint32_t originalFilenameSize;
            CheckAPICall(VerQueryValue(pVersionData, L"\\StringFileInfo\\040904b0\\OriginalFilename",
                                       (void**)&pOriginalFilename, &originalFilenameSize));

            wchar_t* pProductVersion;
            uint32_t productVersionSize;
            CheckAPICall(VerQueryValue(pVersionData, L"\\StringFileInfo\\040904b0\\ProductVersion",
                                       (void**)&pProductVersion, &productVersionSize));

            wchar_t* pFileVersion;
            uint32_t fileVersionSize;
            CheckAPICall(VerQueryValue(pVersionData, L"\\StringFileInfo\\040904b0\\FileVersion", (void**)&pFileVersion,
                                       &fileVersionSize));

            std::wcout << pOriginalFilename << std::endl;
            std::wcout << L"Version: " << pFileVersion << "." << pProductVersion << std::endl;

            delete[] pVersionData;
        }
        else if ((_wcsicmp(args[i].c_str(), L"/?") == 0))
        {
            PrintUsage();
            return;
        }
        // concurrency options
        else if ((_wcsicmp(args[i].c_str(), L"-ConcurrentLoad") == 0))
        {
            ToggleConcurrentLoad(true);
        }
        else if ((_wcsicmp(args[i].c_str(), L"-NumThreads") == 0))
        {
            CheckNextArgument(args, i);
            unsigned num_threads = std::stoi(args[++i].c_str());
            SetNumThreads(num_threads);
        }
        else if ((_wcsicmp(args[i].c_str(), L"-ThreadInterval") == 0))
        {
            CheckNextArgument(args, i);
            unsigned thread_interval = std::stoi(args[++i].c_str());
            SetThreadInterval(thread_interval);
        }
        else if ((_wcsicmp(args[i].c_str(), L"-TopK") == 0))
        {
            CheckNextArgument(args, i);
            SetTopK(std::stoi(args[++i].c_str()));
        }
        else if ((_wcsicmp(args[i].c_str(), L"-GarbageDataMaxValue") == 0))
        {
            CheckNextArgument(args, i);
            SetGarbageDataMaxValue(std::stoul(args[++i].c_str()));
        }
        else if ((_wcsicmp(args[i].c_str(), L"-WaitForDebugger") == 0))
        {
            while (!IsDebuggerPresent())
            {
                Sleep(100);
            }
            __debugbreak();
        }
        else
        {
            std::wstring msg = L"Unknown option ";
            msg += args[i].c_str();
            throw hresult_invalid_argument(msg.c_str());
        }
    }

    if (m_modelPath.empty() && m_modelFolderPath.empty())
    {
        std::cout << std::endl;
        PrintUsage();
        return;
    }

    if (!m_inputData.empty())
    {
        std::transform(m_inputData.begin(), m_inputData.end(), m_inputData.begin(), ::towlower);
        if (m_inputData.find(L".png") != std::string::npos || m_inputData.find(L".jpg") != std::string::npos ||
            m_inputData.find(L".jpeg") != std::string::npos)
        {
            m_imagePaths.push_back(m_inputData);
        }
        else if (m_inputData.find(L".csv") != std::string::npos)
        {
            m_csvData = m_inputData;
        }
        else
        {
            std::wstring msg = L"unknown input type ";
            msg += m_inputData;
            throw hresult_invalid_argument(msg.c_str());
        }
    }
    if (!m_inputImageFolderPath.empty())
    {
        PopulateInputImagePaths();
    }
    SetupOutputDirectories(sBaseOutputPath, sPerfOutputPath, sPerIterationDataPath);

    CheckForInvalidArguments();
}

#pragma warning(pop)

void CommandLineArgs::PopulateInputImagePaths()
{
    for (auto& it : std::filesystem::directory_iterator(m_inputImageFolderPath))
    {
        std::string path = it.path().string();
        if (it.path().string().find(".png") != std::string::npos ||
            it.path().string().find(".jpg") != std::string::npos ||
            it.path().string().find(".jpeg") != std::string::npos)
        {
            std::wstring fileName;
            fileName.assign(path.begin(), path.end());
            m_imagePaths.push_back(fileName);
        }
    }
}

void CommandLineArgs::SetupOutputDirectories(const std::wstring& sBaseOutputPath, const std::wstring& sPerfOutputPath,
                                             const std::wstring& sPerIterationDataPath)
{
    std::filesystem::path PerfOutputPath(sPerfOutputPath);
    std::filesystem::path BaseOutputPath(sBaseOutputPath);
    std::filesystem::path PerIterationDataPath(sPerIterationDataPath);

    if (PerfOutputPath.is_absolute())
    {
        m_perfOutputPath = PerfOutputPath.c_str();
        if (BaseOutputPath.empty())
        {
            BaseOutputPath = PerfOutputPath.remove_filename();
        }
    }

    if (PerIterationDataPath.is_absolute())
    {
        m_perIterationDataPath = PerIterationDataPath.c_str();
        if (BaseOutputPath.empty())
        {
            BaseOutputPath = PerIterationDataPath;
        }
    }

    if (m_perfOutputPath.empty() || m_perIterationDataPath.empty())
    {
        auto time = std::time(nullptr);
        struct tm localTime;
        localtime_s(&localTime, &time);
        std::wostringstream oss;
        oss << std::put_time(&localTime, L"%Y-%m-%d_%H.%M.%S");

        if (BaseOutputPath.empty())
        {
            BaseOutputPath = std::filesystem::current_path();
        }

        if (m_perfOutputPath.empty())
        {
            if (sPerfOutputPath.empty())
#ifdef USE_WINML_NUGET
                PerfOutputPath = L"MicrosoftMLRunner[" + oss.str() + L"].csv";
#else
                PerfOutputPath = L"WinMLRunner[" + oss.str() + L"].csv";
#endif
            PerfOutputPath = BaseOutputPath / PerfOutputPath;
            m_perfOutputPath = PerfOutputPath.c_str();
        }

        if (m_perIterationDataPath.empty())
        {
            if (sPerIterationDataPath.empty())
                PerIterationDataPath = L"PerIterationRun[" + oss.str() + L"]";

            PerIterationDataPath = BaseOutputPath / PerIterationDataPath;
            m_perIterationDataPath = PerIterationDataPath.c_str();
        }
    }
}

void CommandLineArgs::CheckNextArgument(const std::vector<std::wstring>& args, UINT argIdx, UINT checkIdx)
{
    UINT localCheckIdx = checkIdx == 0 ? argIdx + 1 : checkIdx;
    if (localCheckIdx >= args.size() || args[localCheckIdx][0] == L'-')
    {
        std::wstring msg = L"Invalid parameter for ";
        msg += args[argIdx].c_str();
        throw hresult_invalid_argument(msg.c_str());
    }
}

void CommandLineArgs::CheckForInvalidArguments()
{
    if (IsGarbageInput() && IsSaveTensor())
    {
        throw hresult_invalid_argument(L"Cannot save tensor output if no input data is provided!");
    }
    if (m_imagePaths.size() > 1 && IsSaveTensor())
    {
        throw hresult_not_implemented(L"Saving tensor output for multiple images isn't implemented.");
    }
}

std::vector<InputDataType> CommandLineArgs::FetchInputDataTypes()
{
    std::vector<InputDataType> inputDataTypes;

    if (this->UseTensor())
    {
        inputDataTypes.push_back(InputDataType::Tensor);
    }

    if (this->UseRGB())
    {
        inputDataTypes.push_back(InputDataType::ImageRGB);
    }

    if (this->UseBGR())
    {
        inputDataTypes.push_back(InputDataType::ImageBGR);
    }

    return inputDataTypes;
}

std::vector<DeviceType> CommandLineArgs::FetchDeviceTypes()
{
    std::vector<DeviceType> deviceTypes;

    if (this->UseCPU())
    {
        deviceTypes.push_back(DeviceType::CPU);
    }

    if (this->UseGPU())
    {
        deviceTypes.push_back(DeviceType::DefaultGPU);
    }

    if (this->IsUsingGPUHighPerformance())
    {
        deviceTypes.push_back(DeviceType::HighPerfGPU);
    }

    if (this->IsUsingGPUMinPower())
    {
        deviceTypes.push_back(DeviceType::MinPowerGPU);
    }

    return deviceTypes;
}

std::vector<InputBindingType> CommandLineArgs::FetchInputBindingTypes()
{
    std::vector<InputBindingType> inputBindingTypes;

    if (this->UseCPUBoundInput())
    {
        inputBindingTypes.push_back(InputBindingType::CPU);
    }

    if (this->IsUsingGPUBoundInput())
    {
        inputBindingTypes.push_back(InputBindingType::GPU);
    }

    return inputBindingTypes;
}

std::vector<DeviceCreationLocation> CommandLineArgs::FetchDeviceCreationLocations()
{
    std::vector<DeviceCreationLocation> deviceCreationLocations;

    if (this->CreateDeviceInWinML())
    {
        deviceCreationLocations.push_back(DeviceCreationLocation::WinML);
    }

    if (this->IsCreateDeviceOnClient())
    {
        deviceCreationLocations.push_back(DeviceCreationLocation::UserD3DDevice);
    }

    return deviceCreationLocations;
}
void CommandLineArgs::AddPerformanceFileMetadata(const std::string& key, const std::string& value)
{
    // remove commas that may affect processing of CSV
    std::string cleanedValue(value.size(), '0');
    cleanedValue.erase(std::remove_copy(value.begin(), value.end(), cleanedValue.begin(), ','), cleanedValue.end());
    m_perfFileMetadata.push_back(std::make_pair(key, cleanedValue));
}
void CommandLineArgs::AddProvidedInputFeatureValue(const ILearningModelFeatureValue& input)
{
    m_providedInputFeatureValues.push_back(input);
}