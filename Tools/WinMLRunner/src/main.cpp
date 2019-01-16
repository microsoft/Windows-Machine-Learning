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
    for (int i = 0; i < argc; i++)
    {
        string str(argv[i]);
        argsList.push_back(converter.from_bytes(argv[i]));
    }
    CommandLineArgs commandLineArgs(argsList);
    Profiler<WINML_MODEL_TEST_PERF> profiler;
    return run(commandLineArgs, profiler);;
}