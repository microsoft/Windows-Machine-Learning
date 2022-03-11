#include "pch.h"
#include "PixRedistMiddleware.h"
#include "WinMLHelper.h"

using namespace winrt;
using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;
using namespace std;

#ifndef USE_PIX
#define USE_PIX
#endif

static hstring pixCaptureFilePath = L".\\PIXCapture\\capture.wpix";

ID3D12Device* d3d12Device;
ID3D12CommandList* commandList;
ID3D12CommandQueue* commandQueue;
UINT color = PIX_COLOR(255, 0, 0);

void GetHardwareAdapter(IDXGIFactory4* pFactory, IDXGIAdapter1** ppAdapter)
{
    *ppAdapter = nullptr;
    for (UINT adapterIndex = 0; ; ++adapterIndex)
    {
        IDXGIAdapter1* pAdapter = nullptr;
        if (DXGI_ERROR_NOT_FOUND == pFactory->EnumAdapters1(adapterIndex, &pAdapter))
        {
            // No more adapters to enumerate.
            break;
        }

        // Check to see if the adapter supports Direct3D 12, but don't create the
        // actual device yet.
        if (SUCCEEDED(D3D12CreateDevice(pAdapter, D3D_FEATURE_LEVEL_12_0, _uuidof(ID3D12Device), nullptr)))
        {
            *ppAdapter = pAdapter;
            return;
        }
        pAdapter->Release();
    }
}

PIXCaptureParameters GetCap()
{
    PIXCaptureParameters cap;
    cap.GpuCaptureParameters.FileName = pixCaptureFilePath.c_str();
    return cap;
}

// Create command queue & command list to be used by WinML
void GetD3D12CommandAssets()
{
    D3D12_COMMAND_LIST_TYPE type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = type;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    ID3D12CommandAllocator* commandAllocator;

    IDXGIFactory4* factory;
    CreateDXGIFactory1(IID_PPV_ARGS(&factory));

    IDXGIAdapter1* hardwareAdapter;
    GetHardwareAdapter(factory, &hardwareAdapter);

    D3D12CreateDevice(hardwareAdapter, D3D_FEATURE_LEVEL_12_0, _uuidof(ID3D12Device), (void**)&d3d12Device);

    d3d12Device->CreateCommandAllocator(
        type,
        IID_PPV_ARGS(&commandAllocator)
    );
    d3d12Device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueue));
    d3d12Device->CreateCommandList(0, type, commandAllocator, nullptr, __uuidof(ID3D12CommandList), (void**)&commandList);
}

void LoadAndEvaluate(ID3D12CommandQueue* commandQueue)
{
    // Setting markers for each step, these markers will split the commands into sections for easier debugging
    PIXSetMarker(commandQueue, color, "Start loading model...");
    LoadModel();

    PIXSetMarker(commandQueue, color, "Start loading image...");
    LoadImageFile(imagePath);

    PIXSetMarker(commandQueue, color, "Start creating session...");
    CreateSession(commandQueue);

    PIXSetMarker(commandQueue, color, "Start binding model...");
    BindModel();

    PIXSetMarker(commandQueue, color, "Start evaluating model...");
    EvaluateModel();

}

void CaptureWithUserSetMarker()
{
    // Create command queue and command list, this command queue will be used to create model 
    // device and then passed into PIXBeginEvent, PIXEndEvent and PIXSetMarker methods to set 
    // markers. 
    GetD3D12CommandAssets();

    PIXCaptureParameters capParams = GetCap();

    // Start the GPU capture
    PIXBeginCaptureRedist(PIX_CAPTURE_GPU, &capParams);

    // Start PIX event, markers can only be set within Being and End event sections
    PIXBeginEvent(commandQueue, color, "WinMLPIXSample");

    // Do the ML computation
    LoadAndEvaluate(commandQueue);

    // End PIX event
    PIXEndEvent(commandQueue);

    // End capture
    PIXEndCaptureRedist(FALSE);

    printf("Capture successfully completed, please look for capture.wpix file under PIXCapture directory.\n");
}

int main()
{
    TryEnsurePIXFunctions();
    init_apartment();
    CaptureWithUserSetMarker();
    commandList->Release();
    commandQueue->Release();
    d3d12Device->Release();
}
