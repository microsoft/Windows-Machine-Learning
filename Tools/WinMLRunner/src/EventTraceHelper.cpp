#include <direct.h>
#include "EventTraceHelper.h"

#define LOGSESSION_NAME L"winml"

struct __declspec(uuid("{BCAD6AEE-C08D-4F66-828C-4C43461A033D}")) WINML_PROVIDER_GUID_HOLDER;

static const auto WINML_PROVIDER_GUID = __uuidof(WINML_PROVIDER_GUID_HOLDER);

DWORD PointerSize = 0;

/*logman update trace winml -p {BCAD6AEE-C08D-4F66-828C-4C43461A033D} <keyword> <level verbosity> -ets 
To capture only WinML Traces replace <keyword> with 0x1 
To capture only Lotus Profiling Traces replace <keyword> with 0x2 
	Bit 0: WinML Traces 
	Bit 1: Graph profiler 
Level Verbosity is as follows: 
	- (0x0) LogAlways 
	- (0x1) Critical 
	- (0x2) Error 
	- (0x3) Warning 
	- (0x4) Information
	- (0x5) Verbose 
From <https://docs.microsoft.com/en-us/message-analyzer/system-etw-provider-event-keyword-level-settings>  
*/

static void RemoveTrailingSpace(PEVENT_MAP_INFO pMapInfo)
{
    DWORD byteLength = 0;

    for (DWORD i = 0; i < pMapInfo->EntryCount; i++)
    {
        byteLength = (wcslen((LPWSTR)((PBYTE)pMapInfo + pMapInfo->MapEntryArray[i].OutputOffset)) - 1) * 2;
        *((LPWSTR)((PBYTE)pMapInfo + (pMapInfo->MapEntryArray[i].OutputOffset + byteLength))) = L'\0';
    }
}

static DWORD GetArraySize(PEVENT_RECORD pEvent, PTRACE_EVENT_INFO pInfo, USHORT i, PUSHORT ArraySize)
{
    DWORD status = ERROR_SUCCESS;
    PROPERTY_DATA_DESCRIPTOR dataDescriptor;
    DWORD propertySize = 0;

    if ((pInfo->EventPropertyInfoArray[i].Flags & PropertyParamCount) == PropertyParamCount)
    {
        DWORD Count = 0; // Expects the count to be defined by a UINT16 or UINT32
        DWORD j = pInfo->EventPropertyInfoArray[i].countPropertyIndex;
        ZeroMemory(&dataDescriptor, sizeof(PROPERTY_DATA_DESCRIPTOR));
        dataDescriptor.PropertyName = (ULONGLONG)((PBYTE)(pInfo) + pInfo->EventPropertyInfoArray[j].NameOffset);
        dataDescriptor.ArrayIndex = ULONG_MAX;
        status = TdhGetPropertySize(pEvent, 0, NULL, 1, &dataDescriptor, &propertySize);
        status = TdhGetProperty(pEvent, 0, NULL, 1, &dataDescriptor, propertySize, (PBYTE)&Count);
        *ArraySize = (USHORT)Count;
    }
    else
    {
        *ArraySize = pInfo->EventPropertyInfoArray[i].count;
    }

    return status;
}

// Both MOF-based events and manifest-based events can specify name/value maps. The
// map values can be integer values or bit values. If the property specifies a value
// map, get the map.

static DWORD GetMapInfo(PEVENT_RECORD pEvent, LPWSTR pMapName, DWORD DecodingSource,
                                   PEVENT_MAP_INFO& pMapInfo)
{
    DWORD status = ERROR_SUCCESS;
    DWORD mapSize = 0;

    // Retrieve the required buffer size for the map info.

    status = TdhGetEventMapInformation(pEvent, pMapName, pMapInfo, &mapSize);

    if (ERROR_INSUFFICIENT_BUFFER == status)
    {
        pMapInfo = (PEVENT_MAP_INFO)malloc(mapSize);
        if (pMapInfo == NULL)
        {
            printf("Failed to allocate memory for map info (size=%lu).\n", mapSize);
            status = ERROR_OUTOFMEMORY;
            return status;
        }

        // Retrieve the map info.

        status = TdhGetEventMapInformation(pEvent, pMapName, pMapInfo, &mapSize);
    }

    if (ERROR_SUCCESS == status)
    {
        if (DecodingSourceXMLFile == DecodingSource)
        {
            RemoveTrailingSpace(pMapInfo);
        }
    }
    else
    {
        if (ERROR_NOT_FOUND == status)
        {
            status = ERROR_SUCCESS; // This case is okay.
        }
        else
        {
            printf("TdhGetEventMapInformation failed with 0x%x.\n", status);
        }
    }

    return status;
}

static DWORD GetPropertyLength(PEVENT_RECORD pEvent, PTRACE_EVENT_INFO pInfo, USHORT i, PUSHORT PropertyLength)
{
    DWORD status = ERROR_SUCCESS;
    PROPERTY_DATA_DESCRIPTOR dataDescriptor;
    DWORD propertySize = 0;

    // If the property is a binary blob and is defined in a manifest, the property can
    // specify the blob's size or it can point to another property that defines the
    // blob's size. The PropertyParamLength flag tells you where the blob's size is defined.

    if ((pInfo->EventPropertyInfoArray[i].Flags & PropertyParamLength) == PropertyParamLength)
    {
        DWORD Length = 0; // Expects the length to be defined by a UINT16 or UINT32
        DWORD j = pInfo->EventPropertyInfoArray[i].lengthPropertyIndex;
        ZeroMemory(&dataDescriptor, sizeof(PROPERTY_DATA_DESCRIPTOR));
        dataDescriptor.PropertyName = (ULONGLONG)((PBYTE)(pInfo) + pInfo->EventPropertyInfoArray[j].NameOffset);
        dataDescriptor.ArrayIndex = ULONG_MAX;
        status = TdhGetPropertySize(pEvent, 0, NULL, 1, &dataDescriptor, &propertySize);
        status = TdhGetProperty(pEvent, 0, NULL, 1, &dataDescriptor, propertySize, (PBYTE)&Length);
        *PropertyLength = (USHORT)Length;
    }
    else
    {
        if (pInfo->EventPropertyInfoArray[i].length > 0)
        {
            *PropertyLength = pInfo->EventPropertyInfoArray[i].length;
        }
        else
        {
            // If the property is a binary blob and is defined in a MOF class, the extension
            // qualifier is used to determine the size of the blob. However, if the extension
            // is IPAddrV6, you must set the PropertyLength variable yourself because the
            // EVENT_PROPERTY_INFO.length field will be zero.

            if (TDH_INTYPE_BINARY == pInfo->EventPropertyInfoArray[i].nonStructType.InType &&
                TDH_OUTTYPE_IPV6 == pInfo->EventPropertyInfoArray[i].nonStructType.OutType)
            {
                *PropertyLength = (USHORT)sizeof(IN6_ADDR);
            }
            else if (TDH_INTYPE_UNICODESTRING == pInfo->EventPropertyInfoArray[i].nonStructType.InType ||
                     TDH_INTYPE_ANSISTRING == pInfo->EventPropertyInfoArray[i].nonStructType.InType ||
                     (pInfo->EventPropertyInfoArray[i].Flags & PropertyStruct) == PropertyStruct)
            {
                *PropertyLength = pInfo->EventPropertyInfoArray[i].length;
            }
            else
            {
                printf("Unexpected length of 0 for intype %d and outtype %d\n",
                        pInfo->EventPropertyInfoArray[i].nonStructType.InType,
                        pInfo->EventPropertyInfoArray[i].nonStructType.OutType);

                status = ERROR_EVT_INVALID_EVENT_DATA;
            }
        }
    }

    return status;
}

static DWORD GetEventInformation(PEVENT_RECORD pEvent, PTRACE_EVENT_INFO& pInfo)
{
    DWORD status = ERROR_SUCCESS;
    DWORD bufferSize = 0;

    // Retrieve the required buffer size for the event metadata.

    status = TdhGetEventInformation(pEvent, 0, NULL, pInfo, &bufferSize);

    if (ERROR_INSUFFICIENT_BUFFER == status)
    {
        pInfo = (TRACE_EVENT_INFO*)malloc(bufferSize);
        if (pInfo == NULL)
        {
            printf("Failed to allocate memory for event info (size=%lu).\n", bufferSize);
            status = ERROR_OUTOFMEMORY;
            return status;
        }

        // Retrieve the event metadata.

        status = TdhGetEventInformation(pEvent, 0, NULL, pInfo, &bufferSize);
    }

    if (ERROR_SUCCESS != status)
    {
        printf("TdhGetEventInformation failed with 0x%x.\n", status);
    }

    return status;
}

static std::wstring TrimOperatorName(std::wstring& s)
{
    wstring substring = L"_kernel_time";
    int offset = (int)s.find(substring);
    if (offset != -1)
    {
        s.erase(offset, substring.length());
        substring = L"_nchwc";
        offset = (int)s.find(substring);
        if (offset != -1)
        {
            s.erase(offset, substring.length());
        }
    }
    return s;
}

///
/// Return formated property data
///
/// \param EventRecord      The event record received by EventRecordCallback
/// \param EventInfo        A struct with event metadata.
/// \param Index            The index of the property to read.
/// \param pStructureName   Used to retrieve the property if it is inside a struct.
/// \param StructIndex      Used to retrieve the property if it is inside a struct.
/// \param Result           A wide string stream, where the formatted values are appended.
///
/// \return A DWORD with a windows error value. If the function succeded, it returns
///     ERROR_SUCCESS.
///
static DWORD GetFormatedPropertyData(_In_ const PEVENT_RECORD EventRecord,
                                              _In_ const PTRACE_EVENT_INFO EventInfo,
                              _In_ USHORT Index, _Inout_ PBYTE& UserData, _In_ PBYTE EndOfUserData,
                              _Inout_ wstring& propertyValue)
{
    DWORD status = ERROR_SUCCESS;
    DWORD lastMember = 0; // Last member of a structure
    USHORT propertyLength = 0;
    USHORT arraySize = 0;
    DWORD formattedDataSize = 0;
    std::vector<BYTE> formattedData;
    USHORT userDataConsumed = 0;

    status = GetPropertyLength(EventRecord, EventInfo, Index, &propertyLength);
    if (ERROR_SUCCESS != status)
    {
        printf("Failed to query ETW event propery length. Error: %ul", status);
        UserData = NULL;

        return status;
    }

    //
    // Get the size of the array if the property is an array.
    //
    status = GetArraySize(EventRecord, EventInfo, Index, &arraySize);

    for (USHORT k = 0; k < arraySize; k++)
    {
        //
        // If the property is a structure, skip.
        //
        if ((EventInfo->EventPropertyInfoArray[Index].Flags & PropertyStruct) == PropertyStruct)
        {
            continue;
        }
        else if (propertyLength > 0 || (EndOfUserData - UserData) > 0)
        {
            PEVENT_MAP_INFO pMapInfo = NULL;

            //
            // If the property could be a map, try to get its info.
            //
            if (TDH_INTYPE_UINT32 == EventInfo->EventPropertyInfoArray[Index].nonStructType.InType &&
                EventInfo->EventPropertyInfoArray[Index].nonStructType.MapNameOffset != 0)
            {
                status = GetMapInfo(
                    EventRecord,
                    (PWCHAR)((PBYTE)(EventInfo) + EventInfo->EventPropertyInfoArray[Index].nonStructType.MapNameOffset),
                    EventInfo->DecodingSource, pMapInfo);

                if (ERROR_SUCCESS != status)
                {
                    printf("Failed to query ETW event property of type map. Error: %lu", status);

                    if (pMapInfo)
                    {
                        free(pMapInfo);
                        pMapInfo = NULL;
                    }

                    break;
                }
            }

            //
            // Get the size of the buffer required for the formatted data.
            //
            status = TdhFormatProperty(EventInfo, pMapInfo, PointerSize,
                                       EventInfo->EventPropertyInfoArray[Index].nonStructType.InType,
                                       EventInfo->EventPropertyInfoArray[Index].nonStructType.OutType, propertyLength,
                                       (USHORT)(EndOfUserData - UserData), UserData, &formattedDataSize,
                                       (PWCHAR)formattedData.data(), &userDataConsumed);

            if (ERROR_INSUFFICIENT_BUFFER == status)
            {
                formattedData.resize(formattedDataSize);

                //
                // Retrieve the formatted data.
                //
                status = TdhFormatProperty(EventInfo, pMapInfo, PointerSize,
                                           EventInfo->EventPropertyInfoArray[Index].nonStructType.InType,
                                           EventInfo->EventPropertyInfoArray[Index].nonStructType.OutType,
                                           propertyLength, (USHORT)(EndOfUserData - UserData), UserData,
                                           &formattedDataSize, (PWCHAR)formattedData.data(), &userDataConsumed);
            }

            if (pMapInfo)
            {
                free(pMapInfo);
                pMapInfo = NULL;
            }

            if (ERROR_SUCCESS == status)
            {
                propertyValue.assign((PWCHAR)formattedData.data());
                UserData += userDataConsumed;
            }
            else
            {
                printf("Failed to format ETW event property value. Error: %lu", status);
                UserData = NULL;
                break;
            }
        }
    }

    return status;
}

///
/// Formats the data of an event while searching for CPU fallback related events
///
/// \param EventRecord  The event record received by EventRecordCallback
/// \param EventInfo    A struct with event metadata.
/// \param Result       A string with the formatted data.
///
/// \return A DWORD with a windows error value. If the function succeded, it returns
///     ERROR_SUCCESS.
///
static DWORD FormatDataForCPUFallback(_In_ const PEVENT_RECORD EventRecord,
                                               _In_ const PTRACE_EVENT_INFO EventInfo)
{
    DWORD status = ERROR_SUCCESS;

    if (EVENT_HEADER_FLAG_32_BIT_HEADER == (EventRecord->EventHeader.Flags & EVENT_HEADER_FLAG_32_BIT_HEADER))
    {
        PointerSize = 4;
    }
    else
    {
        PointerSize = 8;
    }

    if (EVENT_HEADER_FLAG_STRING_ONLY == (EventRecord->EventHeader.Flags & EVENT_HEADER_FLAG_STRING_ONLY))
    {
        // Do nothing
    }
    else
    {
        PBYTE pUserData = (PBYTE)EventRecord->UserData;
        PBYTE pEndOfUserData = (PBYTE)EventRecord->UserData + EventRecord->UserDataLength;
        PWCHAR pPropertyName;
        wstring operatorType;
        wstring operatorName;
        wstring duration;
        wstring propertyValue;
        wstring executionProvider;
        for (USHORT i = 0; i < EventInfo->TopLevelPropertyCount; i++)
        {
            pPropertyName = (PWCHAR)((PBYTE)(EventInfo) + EventInfo->EventPropertyInfoArray[i].NameOffset);
            status = GetFormatedPropertyData(EventRecord, EventInfo, i, pUserData, pEndOfUserData, propertyValue);
            if (wcscmp(pPropertyName, L"Operator Name") == 0)
            {
                operatorType = propertyValue;
            }
            else if (wcscmp(pPropertyName, L"Event Name") == 0)
            {
                operatorName = TrimOperatorName(propertyValue);
            }
            else if (wcscmp(pPropertyName, L"Duration (us)") == 0)
            {
                duration = propertyValue;
            }
            else if (wcscmp(pPropertyName, L"Execution Provider") == 0)
            {
                executionProvider = propertyValue;
            }
            if (ERROR_SUCCESS != status)
            {
                printf("Failed to format ETW event user data..");

                return status;
            }
        }
        if (wcscmp(executionProvider.c_str(), L"CPUExecutionProvider") == 0 && 
            !operatorName.empty())
        {
            wprintf(L"WARNING: CPU fallback detected for operator %s(%s), duration: %s\n", operatorName.c_str(),
                    operatorType.c_str(), duration.c_str());
        }
    }

    return ERROR_SUCCESS;
}

static VOID WINAPI EventRecordCallback(EVENT_RECORD* pEventRecord)
{
    // This is where you would get the details of ETW event
    DWORD status = ERROR_SUCCESS;
    PTRACE_EVENT_INFO pInfo = NULL;
    LPWSTR pwsEventGuid = NULL;
    PBYTE pUserData = NULL;
    PBYTE pEndOfUserData = NULL;
    ULONGLONG TimeStamp = 0;
    ULONGLONG Nanoseconds = 0;

    // Skips the event if it is the event trace header. Log files contain this event
    // but real-time sessions do not. The event contains the same information as
    // the EVENT_TRACE_LOGFILE.LogfileHeader member that you can access when you open
    // the trace.

    if (IsEqualGUID(pEventRecord->EventHeader.ProviderId, EventTraceGuid) &&
        pEventRecord->EventHeader.EventDescriptor.Opcode == EVENT_TRACE_TYPE_INFO)
    {
        // Skip this event.
    }
    else
    {
        // Process the event. The pEventRecord->UserData member is a pointer to
        // the event specific data, if it exists.

        status = GetEventInformation(pEventRecord, pInfo);

        if (ERROR_SUCCESS != status)
        {
            printf("GetEventInformation failed with %lu\n", status);
        }
        else
        {
            if (DecodingSourceTlg == (pInfo)->DecodingSource)
            {
                FormatDataForCPUFallback(pEventRecord, pInfo);
            }
            else // Not handling any events other than Tlg type
            {
                // Do nothing
            }
        }
    }

    if (pInfo)
    {
        free(pInfo);
    }
}

static ULONG WINAPI BufferCallback(PEVENT_TRACE_LOGFILEW pLogFile) 
{ 
    return TRUE; 
}

void EventTraceHelper::ProcessEventTrace(PTRACEHANDLE traceHandle)
{
    try
    {
        auto status = ProcessTrace(
            traceHandle, 1, 0,
            0); // this call blocks until either the session is stopped or an exception is occurred in event_callback
        if (status != ERROR_SUCCESS && status != ERROR_CANCELLED)
        {
            printf("ProcessTrace failed with %lu\n", status);
        }
    }
    catch (exception ex)
    {
        printf("Exception when processing event trace: %s\n", ex.what());
        // catch exceptions occurred in event_callback
    }
}

void EventTraceHelper::Start()
{
    if (!commandArgs.UseGPU())
    {
        return;
    }

    // Please refer to this link to understand why buffersize was setup in such a seemingly random manner:
    // https://docs.microsoft.com/en-us/windows/win32/api/evntrace/ns-evntrace-event_trace_propertiesbuffersize
    int bufferSize = sizeof(EVENT_TRACE_PROPERTIES) + (sizeof(LOGSESSION_NAME) + 1) * sizeof(wchar_t);
    m_sessionProperties = static_cast<PEVENT_TRACE_PROPERTIES>(malloc(bufferSize));
    ZeroMemory(m_sessionProperties, bufferSize);

    GUID guid;
    UuidCreate(&guid);
    m_sessionProperties->Wnode.BufferSize = static_cast<ULONG>(bufferSize);
    m_sessionProperties->Wnode.Guid = guid;
    m_sessionProperties->Wnode.ClientContext = 0;
    m_sessionProperties->Wnode.Flags = WNODE_FLAG_TRACED_GUID;
    m_sessionProperties->LogFileMode = EVENT_TRACE_REAL_TIME_MODE;
    m_sessionProperties->LoggerNameOffset = sizeof(EVENT_TRACE_PROPERTIES);

    auto hr = StartTrace(static_cast<PTRACEHANDLE>(&m_sessionHandle), LOGSESSION_NAME, m_sessionProperties);
    if (hr != ERROR_SUCCESS)
    {
        printf("Warning starting event trace: Trace already started %d\n", GetLastError());
    }
    else
    {
        auto status = EnableTraceEx2(m_sessionHandle, &WINML_PROVIDER_GUID, EVENT_CONTROL_CODE_ENABLE_PROVIDER,
                                     TRACE_LEVEL_VERBOSE, 2, 0, 0, nullptr);
    }

    EVENT_TRACE_LOGFILE loggerInfo = {};

    TRACE_LOGFILE_HEADER* pHeader = &loggerInfo.LogfileHeader;
    ZeroMemory(&loggerInfo, sizeof(EVENT_TRACE_LOGFILE));
    loggerInfo.LogFileMode = EVENT_TRACE_REAL_TIME_MODE;
    loggerInfo.ProcessTraceMode = PROCESS_TRACE_MODE_REAL_TIME | PROCESS_TRACE_MODE_RAW_TIMESTAMP | PROCESS_TRACE_MODE_EVENT_RECORD;
    loggerInfo.BufferCallback = BufferCallback;

    // provide a callback whenever we get an event record
    loggerInfo.EventRecordCallback = EventRecordCallback;
    loggerInfo.Context = nullptr;
    // LoggerName is the sessionName that we had provided in StartTrace
    // For consuming events from ETL file we will provide path to ETL file.

    loggerInfo.LoggerName = const_cast<LPWSTR>(LOGSESSION_NAME);

    m_traceHandle = OpenTrace(&loggerInfo);
    if (m_traceHandle == INVALID_PROCESSTRACE_HANDLE)
    {
        printf("Error opening event trace: OpenTrace failed with %lu\n", GetLastError());
        throw std::runtime_error("Unable to open trace");
    }

    PTRACEHANDLE pt = static_cast<PTRACEHANDLE>(&m_traceHandle);
    m_threadPool.SubmitWork(ProcessEventTrace, pt);
}

void EventTraceHelper::Stop()
{
    if (!commandArgs.UseGPU())
    {
        return;
    }
    auto status = CloseTrace(m_traceHandle);
    status = ControlTrace(m_sessionHandle, nullptr, m_sessionProperties, EVENT_TRACE_CONTROL_STOP);
    status = EnableTraceEx2(m_sessionHandle, &WINML_PROVIDER_GUID, EVENT_CONTROL_CODE_DISABLE_PROVIDER, 0, 0, 0, 0, nullptr);
}

