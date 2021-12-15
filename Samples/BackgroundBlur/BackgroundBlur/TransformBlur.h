#pragma once


#include <mftransform.h>
#include <mfapi.h>
#include <mftransform.h>
#include <mfidl.h>
#include <mferror.h>
#include <strsafe.h>
#include <shlwapi.h> // registry stuff
#include <atlbase.h>
#include <assert.h>
#include <evr.h>
#include <mfobjects.h>

#include <initguid.h>
#include <uuids.h>      // DirectShow GUIDs
#include <d3d9types.h>
#include <d3d11.h>
#include <dxva2api.h>


#define USE_LOGGING
#include "common/common.h"
using namespace MediaFoundationSamples;
#include "SegmentModel.h"

// Function pointer for the function that transforms the image.
typedef void (*IMAGE_TRANSFORM_FN)(
    BYTE* pDest,
    LONG        lDestStride,
    const BYTE* pSrc,
    LONG        lSrcStride,
    DWORD       dwWidthInPixels,
    DWORD       dwHeightInPixels
    );

class TransformBlur :
    public IMFTransform
{
public: 
    static HRESULT CreateInstance(IUnknown* pUnkOuter, REFIID iid, void** ppSource);

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID iid, void** ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IMFTransform
    STDMETHODIMP GetStreamLimits(
        DWORD* pdwInputMinimum,
        DWORD* pdwInputMaximum,
        DWORD* pdwOutputMinimum,
        DWORD* pdwOutputMaximum
    );

    STDMETHODIMP GetStreamCount(
        DWORD* pcInputStreams,
        DWORD* pcOutputStreams
    );

    STDMETHODIMP GetStreamIDs(
        DWORD   dwInputIDArraySize,
        DWORD* pdwInputIDs,
        DWORD   dwOutputIDArraySize,
        DWORD* pdwOutputIDs
    );

    STDMETHODIMP GetInputStreamInfo(
        DWORD                     dwInputStreamID,
        MFT_INPUT_STREAM_INFO* pStreamInfo
    );

    STDMETHODIMP GetOutputStreamInfo(
        DWORD                     dwOutputStreamID,
        MFT_OUTPUT_STREAM_INFO* pStreamInfo
    );

    STDMETHODIMP GetAttributes(IMFAttributes** ppAttributes);

    STDMETHODIMP GetInputStreamAttributes(
        DWORD           dwInputStreamID,
        IMFAttributes** ppAttributes
    );

    STDMETHODIMP GetOutputStreamAttributes(
        DWORD           dwOutputStreamID,
        IMFAttributes** ppAttributes
    );

    STDMETHODIMP DeleteInputStream(DWORD dwStreamID);

    STDMETHODIMP AddInputStreams(
        DWORD   cStreams,
        DWORD* adwStreamIDs
    );

    STDMETHODIMP GetInputAvailableType(
        DWORD           dwInputStreamID,
        DWORD           dwTypeIndex, // 0-based
        IMFMediaType** ppType
    );

    STDMETHODIMP GetOutputAvailableType(
        DWORD           dwOutputStreamID,
        DWORD           dwTypeIndex, // 0-based
        IMFMediaType** ppType
    );

    STDMETHODIMP SetInputType(
        DWORD           dwInputStreamID,
        IMFMediaType* pType,
        DWORD           dwFlags
    );

    STDMETHODIMP SetOutputType(
        DWORD           dwOutputStreamID,
        IMFMediaType* pType,
        DWORD           dwFlags
    );

    STDMETHODIMP GetInputCurrentType(
        DWORD           dwInputStreamID,
        IMFMediaType** ppType
    );

    STDMETHODIMP GetOutputCurrentType(
        DWORD           dwOutputStreamID,
        IMFMediaType** ppType
    );

    STDMETHODIMP GetInputStatus(
        DWORD           dwInputStreamID,
        DWORD* pdwFlags
    );

    STDMETHODIMP GetOutputStatus(DWORD* pdwFlags);

    STDMETHODIMP SetOutputBounds(
        LONGLONG        hnsLowerBound,
        LONGLONG        hnsUpperBound
    );

    STDMETHODIMP ProcessEvent(
        DWORD              dwInputStreamID,
        IMFMediaEvent* pEvent
    );

    STDMETHODIMP ProcessMessage(
        MFT_MESSAGE_TYPE    eMessage,
        ULONG_PTR           ulParam
    );

    STDMETHODIMP ProcessInput(
        DWORD               dwInputStreamID,
        IMFSample* pSample,
        DWORD               dwFlags
    );

    STDMETHODIMP ProcessOutput(
        DWORD                   dwFlags,
        DWORD                   cOutputBufferCount,
        MFT_OUTPUT_DATA_BUFFER* pOutputSamples, // one per stream
        DWORD* pdwStatus
    );

private: 
    TransformBlur(HRESULT &hr);

    // Destructor is private. The object deletes itself when the reference count is zero.
    ~TransformBlur();

    // HasPendingOutput: Returns TRUE if the MFT is holding an input sample.
    BOOL HasPendingOutput() const { return m_spSample != NULL; }

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

    HRESULT OnGetPartialType(DWORD dwTypeIndex, IMFMediaType** ppmt);
    HRESULT OnCheckInputType(IMFMediaType* pmt);
    HRESULT OnCheckOutputType(IMFMediaType* pmt);
    HRESULT OnCheckMediaType(IMFMediaType* pmt);
    HRESULT OnSetInputType(IMFMediaType* pmt);
    HRESULT OnSetOutputType(IMFMediaType* pmt);
    HRESULT OnProcessOutput(IMFSample** pOut);
    HRESULT OnFlush();

    HRESULT OnSetD3DManager(ULONG_PTR ulParam);
    HRESULT UpdateFormatInfo();
    HRESULT SetupAlloc();
    HRESULT CheckDX11Device(); // Do we need to check the dx11 device in ProcessOutput? 
    HRESULT UpdateDX11Device();
    void InvalidateDX11Resources();
    VideoFrame SampleToVideoFrame(IMFSample* sample, CComPtr<IVideoFrameNativeFactory> factory);
    IDirect3DSurface SampleToD3Dsurface(IMFSample* sample);

    long                        m_nRefCount;                // reference count
    CritSec                     m_critSec;
    bool                        m_bStreamingInitialized;


    IMFAttributes*          m_pAttributes;  // MFT Attributes
    CComPtr<IMFAttributes>  m_spOutputAttributes;

    CComPtr<IMFSample>      m_spSample;                           // Input sample.
    // TODO: Keep an output sample buffer? 
    CComPtr<IMFMediaType>   m_spInputType;                     // Input media type.
    CComPtr<IMFMediaType>   m_spOutputType;                    // Output media type.

    // Fomat information
    FOURCC                      m_videoFOURCC;
    UINT32                      m_imageWidthInPixels;
    UINT32                      m_imageHeightInPixels;
    DWORD                       m_cbImageSize;              // Image size, in bytes.

    // D3D fields
    CComPtr<IMFDXGIDeviceManager>       m_spDeviceManager;
    CComPtr<ID3D11Device>               m_spDevice;
    CComPtr<ID3D11VideoDevice>          m_spVideoDevice;
    CComPtr<ID3D11DeviceContext>        m_spContext;
    HANDLE                              m_hDeviceHandle;          // Handle to the current device
    CComPtr<IMFVideoSampleAllocatorEx> m_spOutputSampleAllocator;

    // Model fields
    SegmentModel  m_segmentModel;
};