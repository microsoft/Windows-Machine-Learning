#pragma once

#include <stdexcept>
#include <wincodec.h>

using namespace DirectX;
using Microsoft::WRL::ComPtr;

class D3D12Quad
{
public:
    D3D12Quad(UINT width, UINT height, std::wstring name);

    void OnInit();
    void OnUpdate();
    void OnRender();
    void OnDestroy();

    // Accessors.
    UINT GetWidth() const { return m_width; }
    UINT GetHeight() const { return m_height; }
    const WCHAR* GetTitle() const { return m_title.c_str(); }

    ComPtr<ID3D12Resource> GetCurrentBuffer();

    bool is_initialized = false;


private:
    static const UINT FrameCount = 2;

    struct Vertex
    {
        XMFLOAT3 position;
        XMFLOAT2 uv;
    };

    // Pipeline objects.
    CD3DX12_VIEWPORT m_viewport;
    CD3DX12_RECT m_scissorRect;
    ComPtr<IDXGISwapChain3> m_swapChain;
    ComPtr<ID3D12Device> m_device;
    ComPtr<ID3D12Resource> m_renderTargets[FrameCount];
    ComPtr<ID3D12CommandAllocator> m_commandAllocator;
    ComPtr<ID3D12CommandQueue> m_commandQueue;
    ComPtr<ID3D12RootSignature> m_rootSignature;
    ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
    ComPtr<ID3D12DescriptorHeap> m_srvHeap;
    ComPtr<ID3D12PipelineState> m_pipelineState;
    ComPtr<ID3D12GraphicsCommandList> m_commandList;
    UINT m_rtvDescriptorSize;

    // App resources.
    ComPtr<ID3D12Resource> m_vertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;

    // Synchronization objects.
    UINT m_frameIndex;
    HANDLE m_fenceEvent;
    ComPtr<ID3D12Fence> m_fence;
    UINT64 m_fenceValue;

    void LoadPipeline();
    void LoadAssets();
    void PopulateCommandList();
    void WaitForPreviousFrame();
    void CreateRootSignature();
    void CreatePipelineState();
    void CreateVertexBuffer();
    void CreateFence();
    void CreateDescriptorHeaps();
    void CreateFrameResources();
    void CreateCurrentBuffer();
    void CopyTextureIntoCurrentBuffer();

    ID3D12Resource* textureBuffer; // the resource heap containing our texture

    int LoadImageDataFromFile(BYTE** imageData, D3D12_RESOURCE_DESC& resourceDescription, LPCWSTR filename, int& bytesPerRow);

    DXGI_FORMAT GetDXGIFormatFromWICFormat(WICPixelFormatGUID& wicFormatGUID);
    WICPixelFormatGUID GetConvertToWICFormat(WICPixelFormatGUID& wicFormatGUID);
    int GetDXGIFormatBitsPerPixel(DXGI_FORMAT& dxgiFormat);
    void LoadImageTexture();
    void Reset();
    void ThrowIfFailed(HRESULT hr);
    void GetHardwareAdapter(
        IDXGIFactory1* pFactory,
        IDXGIAdapter1** ppAdapter,
        bool requestHighPerformanceAdapter = false);

    ID3D12DescriptorHeap* mainDescriptorHeap;
    ID3D12Resource* textureBufferUploadHeap;
    int updateCounter;
    std::vector<std::wstring> fileNames;
    int fileIndex;
    bool justReset;
    UINT m_width;
    UINT m_height;
    float m_aspectRatio;
    std::wstring m_title;
    // holds the texture currently being drawn to the screen in 
    // a resource dimension buffer that will be used by ORT for inference
    ComPtr<ID3D12Resource> currentBuffer;
    int imageBytesPerRow;
};

