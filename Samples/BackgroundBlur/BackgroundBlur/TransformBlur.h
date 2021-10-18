#pragma once
#include <mftransform.h>
#include <mfapi.h>
#include <mftransform.h>
#include <mfidl.h>
#include <mferror.h>
#include <strsafe.h>
#include <shlwapi.h> // registry stuff
#include <winrt/Microsoft.AI.MachineLearning.Experimental.h>
#include <winrt/Microsoft.AI.MachineLearning.h>

#define USE_LOGGING
#include "common.h"
using namespace MediaFoundationSamples;


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

    STDMETHODIMP GetAttributes(IMFAttributes** pAttributes);

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
    BOOL HasPendingOutput() const { return m_pSample != NULL; }

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
    HRESULT OnProcessOutput(IMFMediaBuffer* pIn, IMFMediaBuffer* pOut);
    HRESULT OnFlush();

    HRESULT UpdateFormatInfo();

    long                        m_nRefCount;                // reference count
    CritSec                     m_critSec;

    IMFSample* m_pSample;                 // Input sample.
    IMFMediaType* m_pInputType;              // Input media type.
    IMFMediaType* m_pOutputType;             // Output media type.

    // Fomat information- tbd if needed
    FOURCC                      m_videoFOURCC;
    UINT32                      m_imageWidthInPixels;
    UINT32                      m_imageHeightInPixels;
    DWORD                       m_cbImageSize;              // Image size, in bytes.

};

/*#ifdef USE_LOGGING
#define TRACE_INIT() DebugLog::Initialize()
#define TRACE(x) DebugLog::Trace x
#define TRACE_CLOSE() DebugLog::Close()

// Log HRESULTs on failure.
inline HRESULT _LOG_HRESULT(HRESULT hr, const char* sFileName, long lLineNo)
{
    if (FAILED(hr))
    {
        TRACE((L"%S\n", sFileName));
        TRACE((L"Line: %d hr=0x%X\n", lLineNo, hr));
    }
    return hr;
}

#define LOG_HRESULT(hr) _LOG_HRESULT(hr, __FILE__, __LINE__)
#define LOG_MSG_IF_FAILED(msg, hr) if (FAILED(hr)) { TRACE((msg)); }

#else
#define TRACE_INIT() 
#define TRACE(x) 
#define TRACE_CLOSE()
#define LOG_MSG_IF_FAILED(x, hr)
#define LOG_HRESULT(hr)
#define LOG_MSG_IF_FAILED(msg, hr)
#endif

class AutoLock
{
private:
    CritSec* m_pCriticalSection;
public:
    AutoLock(CritSec& crit)
    {
        m_pCriticalSection = &crit;
        m_pCriticalSection->Lock();
    }
    ~AutoLock()
    {
        m_pCriticalSection->Unlock();
    }
};

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]) )
#endif

class VideoBufferLock
{
public:
    VideoBufferLock(IMFMediaBuffer* pBuffer) : m_p2DBuffer(NULL)
    {
        m_pBuffer = pBuffer;
        m_pBuffer->AddRef();

        // Query for the 2-D buffer interface. OK if this fails.
        m_pBuffer->QueryInterface(IID_IMF2DBuffer, (void**)&m_p2DBuffer);
    }

    ~VideoBufferLock()
    {
        UnlockBuffer();
        SAFE_RELEASE(m_pBuffer);
        SAFE_RELEASE(m_p2DBuffer);
    }

    // LockBuffer:
    // Locks the buffer. Returns a pointer to scan line 0 and returns the stride.

    // The caller must provide the default stride as an input parameter, in case
    // the buffer does not expose IMF2DBuffer. You can calculate the default stride
    // from the media type.

    HRESULT LockBuffer(
        LONG  lDefaultStride,    // Minimum stride (with no padding).
        DWORD dwHeightInPixels,  // Height of the image, in pixels.
        BYTE** ppbScanLine0,    // Receives a pointer to the start of scan line 0.
        LONG* plStride          // Receives the actual stride.
    )
    {
        HRESULT hr = S_OK;

        // Use the 2-D version if available.
        if (m_p2DBuffer)
        {
            hr = m_p2DBuffer->Lock2D(ppbScanLine0, plStride);
        }
        else
        {
            // Use non-2D version.
            BYTE* pData = NULL;

            hr = m_pBuffer->Lock(&pData, NULL, NULL);
            if (SUCCEEDED(hr))
            {
                *plStride = lDefaultStride;
                if (lDefaultStride < 0)
                {
                    // Bottom-up orientation. Return a pointer to the start of the
                    // last row *in memory* which is the top row of the image.
                    *ppbScanLine0 = pData + abs(lDefaultStride) * (dwHeightInPixels - 1);
                }
                else
                {
                    // Top-down orientation. Return a pointer to the start of the
                    // buffer.
                    *ppbScanLine0 = pData;
                }
            }
        }
        return hr;
    }

    HRESULT UnlockBuffer()
    {
        if (m_p2DBuffer)
        {
            return m_p2DBuffer->Unlock2D();
        }
        else
        {
            return m_pBuffer->Unlock();
        }
    }

private:
    IMFMediaBuffer* m_pBuffer;
    IMF2DBuffer* m_p2DBuffer;
};

inline HRESULT GetDefaultStride(IMFMediaType* pType, LONG* plStride)
{
    LONG lStride = 0;

    // Try to get the default stride from the media type.
    HRESULT hr = pType->GetUINT32(MF_MT_DEFAULT_STRIDE, (UINT32*)&lStride);
    if (FAILED(hr))
    {
        // Attribute not set. Try to calculate the default stride.
        GUID subtype = GUID_NULL;

        UINT32 width = 0;
        UINT32 height = 0;

        // Get the subtype and the image size.
        hr = pType->GetGUID(MF_MT_SUBTYPE, &subtype);
        if (SUCCEEDED(hr))
        {
            hr = MFGetAttributeSize(pType, MF_MT_FRAME_SIZE, &width, &height);
        }
        if (SUCCEEDED(hr))
        {
            hr = MFGetStrideForBitmapInfoHeader(subtype.Data1, width, &lStride);
        }

        // Set the attribute for later reference.
        if (SUCCEEDED(hr))
        {
            (void)pType->SetUINT32(MF_MT_DEFAULT_STRIDE, UINT32(lStride));
        }
    }

    if (SUCCEEDED(hr))
    {
        *plStride = lStride;
    }
    return hr;
}*/