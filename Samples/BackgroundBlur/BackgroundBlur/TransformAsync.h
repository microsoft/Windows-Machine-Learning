#pragma once
#include "common/CSampleQueue.h"
#include <Mfidl.h>
#include <mftransform.h>
#include <mfapi.h>
#include <mftransform.h>
#include <mfidl.h>
#include <mferror.h>
#include <strsafe.h>
#include <shlwapi.h> // registry stuff
#include <assert.h>
#include <evr.h>
#include <mfobjects.h>

#include "common.h"
#include <initguid.h>
#include <uuids.h>      // DirectShow GUIDs
#include <d3d9types.h>
#include <d3d11.h>
#include <dxva2api.h>

#define USE_LOGGING
#include "common/common.h"
using namespace MediaFoundationSamples;
#include "SegmentModel.h"

#define MAX_NUM_INPUT_SAMPLES 5

// TODO: Do we need the extension marker? 
// {1F620607-A7FF-4B94-82F4-993F2E17B497}
DEFINE_GUID(TransformAsync_MFSampleExtension_Marker,
    0x1f620607, 0xa7ff, 0x4b94, 0x82, 0xf4, 0x99, 0x3f, 0x2e, 0x17, 0xb4, 0x97);

enum eMFTStatus
{
    MYMFT_STATUS_INPUT_ACCEPT_DATA = 0x00000001,   /* The MFT can accept input data */
    MYMFT_STATUS_OUTPUT_SAMPLE_READY = 0x00000002,
    MYMFT_STATUS_STREAM_STARTED = 0x00000004,
    MYMFT_STATUS_DRAINING = 0x00000008,   /* See http://msdn.microsoft.com/en-us/library/dd940418(v=VS.85).aspx
                                                        ** While the MFT is is in this state, it must not send
                                                        ** any more METransformNeedInput events. It should continue
                                                        ** to send METransformHaveOutput events until it is out of
                                                        ** output. At that time, it should send METransformDrainComplete */
};

class __declspec(uuid("4D354CAD-AA8D-45AE-B031-A85F10A6C655")) TransformAsync;
class TransformAsync :
    public IMFTransform,
    public IMFShutdown,
    public IMFMediaEventGenerator,
    public IMFAsyncCallback,
    public IUnknown
{
public: 
    static HRESULT CreateInstance(IMFTransform** ppMFT);

#pragma region IUnknown
    // IUnknown
    STDMETHODIMP QueryInterface(REFIID iid, void** ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();
#pragma endregion IUnknown

#pragma region IMFTransform
    // IMFTransform
    HRESULT __stdcall   AddInputStreams(
        DWORD   dwStreams,
        DWORD* pdwStreamIDs
    );
    HRESULT __stdcall   DeleteInputStream(
        DWORD   dwStreamID
    );
    HRESULT __stdcall   GetAttributes(
        IMFAttributes** ppAttributes
    );
    HRESULT __stdcall   GetInputAvailableType(
        DWORD           dwInputStreamID,
        DWORD           dwTypeIndex,
        IMFMediaType** ppType
    );
    HRESULT __stdcall   GetInputCurrentType(
        DWORD           dwInputStreamID,
        IMFMediaType** ppType
    );
    HRESULT __stdcall   GetInputStatus(
        DWORD   dwInputStreamID,
        DWORD* pdwFlags
    );
    HRESULT __stdcall   GetInputStreamAttributes(
        DWORD           dwInputStreamID,
        IMFAttributes** ppAttributes
    );
    HRESULT __stdcall   GetInputStreamInfo(
        DWORD                   dwInputStreamID,
        MFT_INPUT_STREAM_INFO* pStreamInfo
    );
    HRESULT __stdcall   GetOutputAvailableType(
        DWORD           dwOutputStreamID,
        DWORD           dwTypeIndex,
        IMFMediaType** ppType
    );
    HRESULT __stdcall   GetOutputCurrentType(
        DWORD           dwOutputStreamID,
        IMFMediaType** ppType
    );
    HRESULT __stdcall   GetOutputStatus(
        DWORD* pdwFlags
    );
    HRESULT __stdcall   GetOutputStreamAttributes(
        DWORD           dwOutputStreamID,
        IMFAttributes** ppAttributes
    );
    HRESULT __stdcall   GetOutputStreamInfo(
        DWORD                   dwOutputStreamID,
        MFT_OUTPUT_STREAM_INFO* pStreamInfo
    );
    HRESULT __stdcall   GetStreamCount(
        DWORD* pdwInputStreams,
        DWORD* pdwOutputStreams
    );
    HRESULT __stdcall   GetStreamIDs(
        DWORD   dwInputIDArraySize,
        DWORD* pdwInputIDs,
        DWORD   dwOutputIDArraySize,
        DWORD* pdwOutputIDs
    );
    HRESULT __stdcall   GetStreamLimits(
        DWORD* pdwInputMinimum,
        DWORD* pdwInputMaximum,
        DWORD* pdwOutputMinimum,
        DWORD* pdwOutputMaximum
    );
    HRESULT __stdcall   ProcessEvent(
        DWORD           dwInputStreamID,
        IMFMediaEvent* pEvent
    );
    HRESULT __stdcall   ProcessInput(
        DWORD       dwInputStreamID,
        IMFSample* pSample,
        DWORD       dwFlags
    );
    HRESULT __stdcall   ProcessMessage(
        MFT_MESSAGE_TYPE eMessage,
        ULONG_PTR ulParam
    );
    HRESULT __stdcall   ProcessOutput(
        DWORD                   dwFlags,
        DWORD                   dwOutputBufferCount,
        MFT_OUTPUT_DATA_BUFFER* pOutputSamples,
        DWORD* pdwStatus
    );
    HRESULT __stdcall   SetInputType(
        DWORD           dwInputStreamID,
        IMFMediaType* pType,
        DWORD           dwFlags
    );
    HRESULT __stdcall   SetOutputBounds(
        LONGLONG hnsLowerBound,
        LONGLONG hnsUpperBound
    );
    HRESULT __stdcall   SetOutputType(
        DWORD           dwOutputStreamID,
        IMFMediaType* pType,
        DWORD           dwFlags
    );

    // **** IMFTransform Helper functions
    // IsValidInputStream: Returns TRUE if dwInputStreamID is a valid input stream identifier.
    BOOL IsValidInputStream(DWORD dwInputStreamID) const
    {
        return dwInputStreamID == 0;
    }

    // IsValidOutputStream: Returns TRUE if dwOutputStreamID is a valid output stream identifier.
    BOOL IsValidOutputStream(DWORD dwOutputStreamID) const
    {
        return dwOutputStreamID == 0;
    }

#pragma endregion IMFTransform

#pragma region IMFShutdown
    // IMFShutdown Implementations
    HRESULT __stdcall   GetShutdownStatus(
        MFSHUTDOWN_STATUS* pStatus
    );
    HRESULT __stdcall   Shutdown(void);
#pragma endregion IMFShutdown

#pragma region IMFMediaEventGenerator
    // IMFMediaEventGenerator Implementations
    HRESULT __stdcall   BeginGetEvent(
        IMFAsyncCallback* pCallback,
        IUnknown* punkState
    );
    HRESULT __stdcall   EndGetEvent(
        IMFAsyncResult* pResult,
        IMFMediaEvent** ppEvent
    );
    HRESULT __stdcall   GetEvent(
        DWORD           dwFlags,
        IMFMediaEvent** ppEvent
    );
    HRESULT __stdcall   QueueEvent(
        MediaEventType      met,
        REFGUID             guidExtendedType,
        HRESULT             hrStatus,
        const PROPVARIANT* pvValue
    );
#pragma endregion IMFMediaEventGenerator

#pragma region IMFAsyncCallback
    // IMFAsyncCallback Implementations
    HRESULT __stdcall   GetParameters(
        DWORD* pdwFlags,
        DWORD* pdwQueue
    );
    HRESULT __stdcall   Invoke(
        IMFAsyncResult* pAsyncResult
    );
#pragma endregion IMFAsyncCallback

    HRESULT             SubmitEval(IMFSample* pInputSample);
    HRESULT             FinishEval(winrt::com_ptr<IMFSample> pInputSample, winrt::com_ptr<IMFSample> pOutputSample,
        IDirect3DSurface src, IDirect3DSurface dest, LONGLONG hnsDuration, LONGLONG hnsTime, UINT64 pun64MarkerID);

protected: 
    TransformAsync(HRESULT& hr);

    // Destructor is private. The object deletes itself when the reference count is zero.
    ~TransformAsync();
    HRESULT InitializeTransform(void);
    HRESULT             ShutdownEventQueue(void);

    /******* MFT Helpers **********/
    HRESULT OnGetPartialType(DWORD dwTypeIndex, IMFMediaType** ppmt);
    HRESULT OnCheckInputType(IMFMediaType* pmt);
    HRESULT OnCheckOutputType(IMFMediaType* pmt);
    HRESULT OnCheckMediaType(IMFMediaType* pmt);
    HRESULT OnSetInputType(IMFMediaType* pmt);
    HRESULT OnSetOutputType(IMFMediaType* pmt);

    HRESULT OnSetD3DManager(ULONG_PTR ulParam);
    HRESULT UpdateFormatInfo(); // TODO: We want this prob for model change right? 
    HRESULT SetupAlloc(); // TODO: Prob don't need allocator anymore right? 
    HRESULT CheckDX11Device(); // Do we need to check the dx11 device in ProcessOutput? 
    HRESULT UpdateDX11Device();
    void InvalidateDX11Resources();
    IDirect3DSurface SampleToD3Dsurface(IMFSample* sample);


    /******* MFT Media Event Handlers **********/
    HRESULT             OnStartOfStream(void);
    HRESULT             OnEndOfStream(void);
    HRESULT             OnDrain(
        const UINT32 un32StreamID
    );
    HRESULT             OnFlush(void);
    HRESULT             OnMarker(
        const ULONG_PTR pulID
    );

    /***********End Event Handlers************/
    HRESULT             RequestSample(
        const UINT32 un32StreamID
    );
    HRESULT             FlushSamples(void);
    HRESULT             ScheduleFrameInference(void);
    HRESULT             SendEvent(void);
    BOOL                IsLocked(void);
    BOOL                IsMFTReady(void); // TODO: Add calls in ProcessOutput, SetInput/OutputType

    // Member variables
    volatile ULONG                  m_nRefCount;            // reference count
    CritSec                         m_critSec;
    winrt::com_ptr<IMFMediaType>    m_spInputType;          // Input media type
    winrt::com_ptr<IMFMediaType>    m_spOutputType;         // Output media type
    winrt::com_ptr<IMFAttributes>   m_spAttributes;         // MFT Attributes
    winrt::com_ptr<IMFAttributes>   m_spOutputAttributes;   // TODO: Sample allocator attributes
    bool                            m_bStreamingInitialized;
    volatile    ULONG               m_ulSampleCounter;      // Frame number, can use to pick a IStreamModel
    volatile ULONG m_ulProcessedFrameNum; // Number of frames we've processed
    volatile ULONG m_currFrameNumber; // The current frame to be processing



    // Event fields
    DWORD                           m_dwStatus;
    winrt::com_ptr<IMFMediaEventQueue> m_pEventQueue;
    DWORD               m_dwNeedInputCount;
    DWORD               m_dwHaveOutputCount;
    DWORD               m_dwDecodeWorkQueueID;
    LONGLONG            m_llCurrentSampleTime;
    BOOL                m_bFirstSample;
    BOOL                m_bShutdown;
    CSampleQueue* m_pInputSampleQueue;
    CSampleQueue* m_pOutputSampleQueue;


    // Fomat information
    FOURCC                          m_videoFOURCC;
    UINT32                          m_imageWidthInPixels;
    UINT32                          m_imageHeightInPixels;
    DWORD                           m_cbImageSize;          // Image size, in bytes.

    // D3D fields
    winrt::com_ptr<IMFDXGIDeviceManager>       m_spDeviceManager;
    winrt::com_ptr<ID3D11Device>               m_spDevice;
    winrt::com_ptr<ID3D11VideoDevice>          m_spVideoDevice;
    winrt::com_ptr<ID3D11DeviceContext>        m_spContext;
    HANDLE                                     m_hDeviceHandle;          // Handle to the current device
    winrt::com_ptr<IMFVideoSampleAllocatorEx>  m_spOutputSampleAllocator;

    // Model Inference fields
    // TODO: Prob needs to be a vector so can dynamically allocate based on what numThreads ends up as.
    std::vector<std::unique_ptr<IStreamModel>> m_models; 
    std::condition_variable thread; // Used to notify a thread of an available job
    int m_numThreads = 5;//std::thread::hardware_concurrency(); // TODO: See if this actually is helpful


    int swapChainEntry = 0;
    std::mutex Processing;

    // Pseudocode
    // int numThreads; needs to be configured by constructor
    // std::array<unique_ptr<IStreamModel>>(numThreads) one streamModel for each thread
    // something for desired resolution?? if model has free dimensions


};

HRESULT DuplicateAttributes(
    IMFAttributes* pDest,
    IMFAttributes* pSource
);