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
    TRACEHANDLE m_traceHandle;
    TRACEHANDLE m_sessionHandle;
    PEVENT_TRACE_PROPERTIES m_sessionProperties;
    ThreadPool m_threadPool;
    CommandLineArgs commandArgs;

public:
    EventTraceHelper(CommandLineArgs args) : m_sessionHandle(INVALID_PROCESSTRACE_HANDLE), m_threadPool(1)
    {
        commandArgs = args;
    }

    void Start();
    void Stop();

private:
    static void ProcessEventTrace(PTRACEHANDLE traceHandle);

};