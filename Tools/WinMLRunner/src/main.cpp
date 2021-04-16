#include "CommandLineArgs.h"
#include "Run.h"
#include "Common.h"
#include "Windows.AI.MachineLearning.Native.h"
#include <iostream>
#include <codecvt>
#include <thread>
using namespace std;

void PopulateSessionOptions(LearningModelSessionOptions& sessionOptions, const CommandLineArgs& args)
{
    // Batch Size Override as 1
    try
    {
        sessionOptions.BatchSizeOverride(1);
    }
    catch (...)
    {
        printf("Batch size override couldn't be set.\n");
        throw;
    }

    if (args.CPUThrottle())
    {
        // calculate the number of processor cores
        // adapted from https://docs.microsoft.com/en-us/windows/win32/api/sysinfoapi/nf-sysinfoapi-getlogicalprocessorinformation
        DWORD byteOffset = 0;
        DWORD processorCoreCount = 0;
        PSYSTEM_LOGICAL_PROCESSOR_INFORMATION buffer = NULL;
        PSYSTEM_LOGICAL_PROCESSOR_INFORMATION ptr = NULL;
        DWORD returnLength = 0;
        bool done = false;

        // have to call it twice, once to get the length and once to fill the buffer
        GetLogicalProcessorInformation(buffer, &returnLength);
        buffer = new SYSTEM_LOGICAL_PROCESSOR_INFORMATION[returnLength];
        GetLogicalProcessorInformation(buffer, &returnLength);

        ptr = buffer;
        while (byteOffset + sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION) <= returnLength)
        {
            if (ptr->Relationship == RelationProcessorCore)
            {
                processorCoreCount++;
            }
            byteOffset += sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
            ptr++;
        }

        // Set the number of intra op threads to half of processor cores.
        uint32_t desiredThreads = processorCoreCount / 2;
        auto nativeOptions = sessionOptions.as<ILearningModelSessionOptionsNative>();
        nativeOptions->SetIntraOpNumThreadsOverride(desiredThreads);

        delete[] buffer;
    }
}

int main(int argc, char *argv[])
{
    std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
    vector<wstring> argsList;
    CommandLineArgs* commandLineArgs = NULL;
    for (int i = 1; i < argc; i++)
    {
        string str(argv[i]);
        argsList.push_back(converter.from_bytes(argv[i]));
    }
    try
    {
        commandLineArgs = new CommandLineArgs(argsList);
    }
    catch (hresult_error hr)
    {
        std::cout << "Creating CommandLineArgs Failed. Invalid Arguments." << std::endl;
        std::wcout << hr.message().c_str() << std::endl;
        return hr.code();
    }
    Profiler<WINML_MODEL_TEST_PERF> profiler;
    vector<LearningModelDeviceWithMetadata> deviceList;
    try
    {
        PopulateLearningModelDeviceList(*commandLineArgs, deviceList);
    }
    catch (const hresult_error& error)
    {
        wprintf(error.message().c_str());
        return error.code();
    }
    LearningModelSessionOptions sessionOptions;
    PopulateSessionOptions(sessionOptions, *commandLineArgs);
    int returnCode = run(*commandLineArgs, profiler, deviceList, sessionOptions);
    delete commandLineArgs;
    return returnCode;
}
