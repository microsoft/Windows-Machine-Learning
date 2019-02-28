#include "CommandLineArgs.h"
#include "Run.h"
#include "Common.h"
#include <iostream>
#include <codecvt>
using namespace std;

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
    int returnCode = run(*commandLineArgs, profiler);
    free(commandLineArgs);
    return returnCode;
}