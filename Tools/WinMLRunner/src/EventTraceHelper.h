#pragma once
#include <iostream>
#include <windows.h>
#include <evntrace.h>
#include <evntcons.h>
#include <tchar.h>
#include <tdh.h>
#include "ThreadPool.h"
#include <in6addr.h>
#include "CommandLineArgs.h"

using namespace std;

class EventTraceHelper
{
private:
    TRACEHANDLE traceHandle;
    TRACEHANDLE sessionHandle;
    PEVENT_TRACE_PROPERTIES sessionProperties;
    ThreadPool threadPool;
    static DWORD PointerSize;
    static CommandLineArgs commandArgs;

public:
    EventTraceHelper(CommandLineArgs args) : sessionHandle(INVALID_PROCESSTRACE_HANDLE), threadPool(1)
    {
        commandArgs = args;
    }

    void Start();
    void Stop();

private:
    static void ProcessEventTrace(PTRACEHANDLE traceHandle);
    static ULONG WINAPI BufferCallback(PEVENT_TRACE_LOGFILEW pLogFile);
    static VOID WINAPI EventRecordCallback(EVENT_RECORD* pEventRecord);
    
    static void RemoveTrailingSpace(PEVENT_MAP_INFO pMapInfo);
    static DWORD GetArraySize(PEVENT_RECORD pEvent, PTRACE_EVENT_INFO pInfo, USHORT i, PUSHORT ArraySize);
    static DWORD GetMapInfo(PEVENT_RECORD pEvent, LPWSTR pMapName, DWORD DecodingSource, PEVENT_MAP_INFO& pMapInfo);
    static DWORD GetPropertyLength(PEVENT_RECORD pEvent, PTRACE_EVENT_INFO pInfo, USHORT i, PUSHORT PropertyLength);
    static DWORD GetEventInformation(PEVENT_RECORD pEvent, PTRACE_EVENT_INFO& pInfo);
    static wstring& TrimOperatorName(wstring& s);
    static DWORD GetFormatedPropertyData(_In_ const PEVENT_RECORD EventRecord, _In_ const PTRACE_EVENT_INFO EventInfo,
                                         _In_ USHORT Index, _Inout_ PBYTE& UserData, _In_ PBYTE EndOfUserData,
                                         _Inout_ wstring& propertyValue);
    static DWORD FormatDataForCPUFallback(_In_ const PEVENT_RECORD EventRecord, _In_ const PTRACE_EVENT_INFO EventInfo);

};