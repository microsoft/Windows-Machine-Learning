#pragma once
#include "External/CSampleQueue.h"
#include <Mfidl.h>
#include <mftransform.h>
#include <mfapi.h>
#include <mftransform.h>
#include <mfidl.h>
#include <mmiscapi.h>
#include <mferror.h>
#include <strsafe.h>
#include <shlwapi.h> // registry stuff
#include <assert.h>
#include <mfobjects.h>

#include <initguid.h>
#include <uuids.h>      // DirectShow GUIDs
#include <d3d9types.h>
#include <d3d11.h>
#include <d3d11_3.h>
#include <d3d11_4.h>
#include <dxva2api.h>
#include "External/common.h"
#include "SegmentModel.h"

#define USE_LOGGING
#define MFT_NUM_DEFAULT_ATTRIBUTES  4
#define FRAME_RATE_UPDATE 30     // The number of samples to render before update framerate value


DWORD __stdcall FrameThreadProc(LPVOID lpParam);
enum eMFTStatus
{
    MYMFT_STATUS_INPUT_ACCEPT_DATA = 0x00000001,   /* The MFT can accept input data */
    MYMFT_STATUS_OUTPUT_SAMPLE_READY = 0x00000002,
    MYMFT_STATUS_STREAM_STARTED = 0x00000004,
    MYMFT_STATUS_DRAINING = 0x00000008,   /*   See http://msdn.microsoft.com/en-us/library/dd940418(v=VS.85).aspx
                                            ** While the MFT is is in this state, it must not send
                                            ** any more METransformNeedInput events. It should continue
                                            ** to send METransformHaveOutput events until it is out of
                                            ** output. At that time, it should send METransformDrainComplete */
};

// This class is exported from the dll
class __declspec(uuid("4D354CAD-AA8D-45AE-B031-A85F10A6C655")) TransformAsync;
class TransformAsync:
    public IMFTransform,                // Main interface to implement for MFT functionality. 
    public IMFShutdown,                 // Shuts down the MFT event queue when signalled from client. 
    public IMFMediaEventGenerator,      // Generates NeedInput and HasOutput events for the client to respond to. 
    public IMFAsyncCallback,            // The callback interface to notify when an async method completes. 
    public IUnknown,
    public IMFVideoSampleAllocatorNotify // Callback interface to notify when an IMFSample is freed back to allocator. 
{
public: 
    static HRESULT CreateInstance(IMFTransform** ppMFT) noexcept;

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
    // IsValidInputStream: Returns true if dwInputStreamID is a valid input stream identifier.
    bool IsValidInputStream(DWORD dwInputStreamID) const
    {
        return dwInputStreamID == 0;
    }

    // IsValidOutputStream: Returns true if dwOutputStreamID is a valid output stream identifier.
    bool IsValidOutputStream(DWORD dwOutputStreamID) const
    {
        return dwOutputStreamID == 0;
    }

#pragma endregion IMFTransform

#pragma region IMFShutdown
    // IMFShutdown Implementations
    HRESULT __stdcall   GetShutdownStatus(
        MFSHUTDOWN_STATUS* pStatus
    );
    HRESULT __stdcall   Shutdown();
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

#pragma region IMFVideoSampleAllocatorNotify
    HRESULT STDMETHODCALLTYPE NotifyRelease();
#pragma endregion IMFVideoSampleAllocatorNotify

    // Uses the next available StreamModelBase to run inference on pInputSample 
    // and allocates a transformed output sample. 
    HRESULT SubmitEval(IMFSample* pInputSample);

    // Helper function for SubmitEval, sets attributes on the output sample,
    // adds it to the output sample queue, and queues an MFHasOutput event. 
    HRESULT FinishEval(
        IMFSample* pOutputSample,
        LONGLONG hnsDuration,
        LONGLONG hnsTime,
        UINT64 pun64MarkerID);

    void WriteFrameRate(const WCHAR* frameRate);
    void SetFrameRateWnd(HWND hwnd);
    void SetModelBasePath(winrt::hstring path);
    wil::unique_handle m_fenceEvent;      // Handle to the fence complete event
    
protected:

    // Destructor is private. The object deletes itself when the reference count is zero.
    ~TransformAsync();

    // Called by the constructor, intializes sample queues, transform attributes,
    // and the event queue. 
    HRESULT InitializeTransform();
    HRESULT ShutdownEventQueue();

    /******* MFT Helpers **********/
    // Returns the partial media type of the media type in g_mediaSubtypes.
    HRESULT OnGetPartialType(DWORD dwTypeIndex, IMFMediaType** ppmt);

    // Validates the input media type.
    HRESULT OnCheckInputType(IMFMediaType* pmt);
    // Validates the output media type.
    HRESULT OnCheckOutputType(IMFMediaType* pmt);
    // Validates the media type against supported types listed in g_mediaSubtypes.
    HRESULT OnCheckMediaType(IMFMediaType* pmt);

    //  Sets or clears the input media type once input type has been validated. 
    HRESULT OnSetInputType(IMFMediaType* pmt);
    // Sets or clears the input media type once output type has been validated. 
    HRESULT OnSetOutputType(IMFMediaType* pmt);
    // Sets m_deviceManager to the IMFDXGIDeviceManager that ulParam points to. 
    HRESULT OnSetD3DManager(ULONG_PTR ulParam);

    // After the input type is set, update MFT format information and sets
    // StreamModelBase input sizes. 
    HRESULT UpdateFormatInfo();

    // Sets up the output sample allocator.
    HRESULT SetupAlloc();
    // Tests the device manager to ensure it's still available. 
    HRESULT CheckDX11Device();
    // Updates the device handle if it's no longer valid. 
    HRESULT UpdateDX11Device();
    void    InvalidateDX11Resources();
    IDirect3DSurface SampleToD3Dsurface(IMFSample* sample);

    /******* MFT Media Event Handlers **********/
    HRESULT             OnStartOfStream();
    HRESULT             OnEndOfStream();
    HRESULT             OnDrain(
        const UINT32 un32StreamID
    );
    HRESULT             OnFlush();
    HRESULT             OnMarker(
        const ULONG_PTR pulID
    );

    /*********** End Event Handlers ************/
    // Queues an MFNeedInput event for the client to respond to. 
    HRESULT             RequestSample(const UINT32 un32StreamID);
    HRESULT             FlushSamples();
    // Pops a sample from the input queue and schedules an inference job
    // with the Media Foundation async work queue. 
    HRESULT             ScheduleFrameInference();
    bool                IsMFTReady();

    // Member variables
    volatile ULONG                  m_refCount = 1;        // Reference count.
    std::recursive_mutex            m_mutex;                // Controls access streaming status and event queue. 
    com_ptr<IMFMediaType>           m_inputType;          // Input media type.
    com_ptr<IMFMediaType>           m_outputType;         // Output media type.
    com_ptr<IMFAttributes>          m_attributes;         // MFT Attributes.
    com_ptr<IMFAttributes>          m_allocatorAttributes;// Output sample allocator attributes.    
    bool                            m_allocatorInitialized;// True if sample allocator has been initialized. 
    volatile ULONG                  m_sampleCounter = 0;  // Frame number, can use to pick a StreamModelBase.
    volatile ULONG                  m_processedFrameNum;  // Number of frames we've processed.
    volatile ULONG                  m_currFrameNumber;      // The current frame to be processed.

    // Event fields
    DWORD                           m_status = 0;         // MFT status. References one of the states defined in enum eMFTStatus.
    com_ptr<IMFMediaEventQueue>     m_eventQueue;         // Event queue the client uses to get NeedInput/HaveOutput events via IMFMediaEventGenerator.
    DWORD                           m_needInputCount = 0; // Number of pending NeedInput requests to be resolved by client calling ProcessInput. 0 at end of stream.
    DWORD                           m_haveOutputCount = 0;// Number of pending HaveOutput requests to be resolved by client calling ProcessOutput.
    bool                            m_firstSample = true;  // True the incoming sample is the first one after a gap in the stream. 
    bool                            m_shutdown = false;    // True if MFT is currently shutting down, signals to client through IMFShutdown. 
    CSampleQueue* m_inputSampleQueue;    // Queue of input samples to be processed by ProcessInput. 
    CSampleQueue* m_outputSampleQueue;   // Queue of output samples to be processed by ProcessOutput. 

    // Fomat information
    FOURCC                          m_videoFOURCC = 0;      // Video format code. 
    UINT32                          m_imageWidthInPixels = 0;
    UINT32                          m_imageHeightInPixels = 0;
    DWORD                           m_imageSize;          // Image size, in bytes.

    // D3D fields
    com_ptr<IMFDXGIDeviceManager>       m_deviceManager;  // Device manager, shared with the video renderer. 
    wil::unique_handle                  m_deviceHandle;    // Handle to the current device
    // Immediate device context
    com_ptr<ID3D11DeviceContext4>       m_context;
    com_ptr<ID3D11Device5>              m_device;
    com_ptr<IMFVideoSampleAllocatorEx>  m_outputSampleAllocator;  // Allocates d3d-backed output samples. 
    com_ptr<IMFVideoSampleAllocatorCallback> m_outputSampleCallback; // Callback for when a frame is returns to the output sample allocator. 
    // Frame rate synch objects
    com_ptr<ID3D11Fence>                m_fence;
    UINT64 m_fenceValue;
    wil::unique_handle                  m_frameThread;
    HWND                                m_frameWnd;

    // Model Inference fields
    int m_numThreads = std::thread::hardware_concurrency(); // Number of threads running inference in parallel.
    std::vector<std::unique_ptr<StreamModelBase>> m_models; // m_numThreads number of models to run inference in parallel. 
    int m_modelIndex = 0;
    winrt::hstring m_modelPath;
};
