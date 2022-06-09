#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN    // Exclude rarely-used stuff from Windows headers.
#endif

#include <windows.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <D3Dcompiler.h>
#include <DirectXMath.h>
#include "d3dx12.h"
#include <string>
#include <wincodec.h>

// this will only call release if an object exists (prevents exceptions calling release on non existant objects)
#define SAFE_RELEASE(p) { if ( (p) ) { (p)->Release(); (p) = 0; } }

using namespace DirectX; // we will be using the directxmath library

// create and show the window
bool InitializeWindow(HINSTANCE &hInstance,
    int ShowWnd,
    bool fullscreen,
    HWND &hwnd,
    int Width,
    int Height,
    LPCTSTR WindowName,
    LPCTSTR WindowTitle);

// callback function for windows messages
LRESULT CALLBACK WndProc(HWND hWnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam);

struct ConstantBufferPerObject {
    XMFLOAT4X4 wvpMat;
};

// Constant buffers must be 256-byte aligned which has to do with constant reads on the GPU.
// We are only able to read at 256 byte intervals from the start of a resource heap, so we will
// make sure that we add padding between the two constant buffers in the heap (one for cube1 and one for cube2)
// Another way to do this would be to add a float array in the constant buffer structure for padding. In this case
// we would need to add a float padding[50]; after the wvpMat variable. This would align our structure to 256 bytes (4 bytes per float)
// The reason i didn't go with this way, was because there would actually be wasted cpu cycles when memcpy our constant
// buffer data to the gpu virtual address. currently we memcpy the size of our structure, which is 16 bytes here, but if we
// were to add the padding array, we would memcpy 64 bytes if we memcpy the size of our structure, which is 50 wasted bytes
// being copied.
int ConstantBufferPerObjectAlignedSize = (sizeof(ConstantBufferPerObject) + 255) & ~255;

ConstantBufferPerObject cbPerObject; // this is the constant buffer data we will send to the gpu 
                                        // (which will be placed in the resource we created above)

// initializes direct3d 12
bool InitD3D(bool& Running,
    ID3D12Device*& device,
    ID3D12CommandQueue*& commandQueue,
    int Width,
    int Height,
    const int frameBufferCount,
    HWND& hwnd,
    bool FullScreen,
    IDXGISwapChain3*& swapChain,
    int& frameIndex,
    ID3D12DescriptorHeap*& rtvDescriptorHeap,
    int& rtvDescriptorSize,
    ID3D12Resource* renderTargets[],
    ID3D12CommandAllocator* commandAllocator[],
    ID3D12GraphicsCommandList*& commandList,
    ID3D12Fence* fence[],
    UINT64 (&fenceValue)[],
    HANDLE& fenceEvent,
    ID3D12RootSignature*& rootSignature,
    ID3D12PipelineState*& pipelineStateObject,
    ID3D12Resource*& vertexBuffer,
    int& numCubeIndices,
    ID3D12Resource*& indexBuffer,
    ID3D12DescriptorHeap*& dsDescriptorHeap,
    ID3D12Resource*& depthStencilBuffer,
    ID3D12Resource* constantBufferUploadHeaps[],
    UINT8* cbvGPUAddress[],
    ID3D12Resource*& textureBuffer,
    ID3D12Resource*& textureBufferUploadHeap,
    ID3D12DescriptorHeap*& mainDescriptorHeap,
    D3D12_VERTEX_BUFFER_VIEW& vertexBufferView,
    D3D12_INDEX_BUFFER_VIEW& indexBufferView,
    D3D12_VIEWPORT& viewport,
    D3D12_RECT& scissorRect,
    XMFLOAT4X4& cameraProjMat,
    XMFLOAT4& cameraPosition,
    XMFLOAT4& cameraTarget,
    XMFLOAT4& cameraUp,
    XMFLOAT4X4& cameraViewMat,
    XMFLOAT4& cube1Position,
    XMFLOAT4X4& cube1RotMat,
    XMFLOAT4X4& cube1WorldMat,
    XMFLOAT4X4& cube2RotMat,
    XMFLOAT4X4& cube2WorldMat,
    int ConstantBufferPerObjectAlignedSize);

int LoadImageDataFromFile(BYTE** imageData,
    D3D12_RESOURCE_DESC& resourceDescription,
    LPCWSTR filename,
    int& bytesPerRow);

// get the dxgi format equivilent of a wic format
DXGI_FORMAT GetDXGIFormatFromWICFormat(WICPixelFormatGUID& wicFormatGUID);

// get the number of bits per pixel for a dxgi format
int GetDXGIFormatBitsPerPixel(DXGI_FORMAT& dxgiFormat);

// get a dxgi compatible wic format from another wic format
WICPixelFormatGUID GetConvertToWICFormat(WICPixelFormatGUID& wicFormatGUID);

// update the game logic
void Update(XMFLOAT4X4& cube1RotMat,
    XMFLOAT4& cube1Position,
    XMFLOAT4X4& cube1WorldMat,
    XMFLOAT4X4& cameraViewMat,
    XMFLOAT4X4& cameraProjMat,
    UINT8* cbvGPUAddress[],
    int& frameIndex,
    XMFLOAT4X4& cube2RotMat,
    XMFLOAT4& cube2PositionOffset,
    XMFLOAT4X4& cube2WorldMat);

// update the direct3d pipeline (update command lists)
CD3DX12_CPU_DESCRIPTOR_HANDLE UpdatePipeline(ID3D12CommandAllocator* commandAllocator[],
    int& frameIndex,
    bool& Running,
    ID3D12GraphicsCommandList*& commandList,
    ID3D12PipelineState*& pipelineStateObject,
    ID3D12Resource* renderTargets[],
    ID3D12DescriptorHeap*& rtvDescriptorHeap,
    ID3D12DescriptorHeap*& dsDescriptorHeap,
    int& rtvDescriptorSize,
    IDXGISwapChain3*& swapChain,
    ID3D12Fence* fence[],
    UINT64(&fenceValue)[],
    HANDLE& fenceEvent,
    ID3D12RootSignature*& rootSignature,
    ID3D12DescriptorHeap*& mainDescriptorHeap,
    D3D12_VIEWPORT& viewport,
    D3D12_RECT& scissorRect,
    D3D12_VERTEX_BUFFER_VIEW& vertexBufferView,
    D3D12_INDEX_BUFFER_VIEW& indexBufferView,
    ID3D12Resource* constantBufferUploadHeaps[]);

// execute the command list
CD3DX12_CPU_DESCRIPTOR_HANDLE Render(ID3D12GraphicsCommandList*& commandList,
    ID3D12CommandAllocator* commandAllocator[],
    ID3D12CommandQueue*& commandQueue,
    ID3D12Fence* fence[],
    int& frameIndex,
    UINT64(&fenceValue)[],
    bool& Running,
    IDXGISwapChain3*& swapChain,
    ID3D12PipelineState*& pipelineStateObject,
    ID3D12Resource* renderTargets[],
    ID3D12DescriptorHeap*& rtvDescriptorHeap,
    ID3D12DescriptorHeap*& dsDescriptorHeap,
    int& rtvDescriptorSize,
    HANDLE& fenceEvent,
    ID3D12RootSignature*& rootSignature,
    ID3D12DescriptorHeap*& mainDescriptorHeap,
    D3D12_VIEWPORT& viewport,
    D3D12_RECT& scissorRect,
    D3D12_VERTEX_BUFFER_VIEW& vertexBufferView,
    D3D12_INDEX_BUFFER_VIEW& indexBufferView,
    ID3D12Resource* constantBufferUploadHeaps[],
    int& numCubeIndices);

void Cleanup(const int frameBufferCount,
    int& frameIndex,
    IDXGISwapChain3*& swapChain,
    ID3D12Device*& device,
    ID3D12CommandQueue*& commandQueue,
    ID3D12DescriptorHeap*& rtvDescriptorHeap,
    ID3D12GraphicsCommandList*& commandList,
    ID3D12Resource* renderTargets[],
    ID3D12CommandAllocator* commandAllocator[],
    ID3D12Fence* fence[],
    ID3D12PipelineState*& pipelineStateObject,
    ID3D12RootSignature*& rootSignature,
    ID3D12Resource*& vertexBuffer,
    ID3D12Resource*& indexBuffer,
    ID3D12Resource*& depthStencilBuffer,
    ID3D12DescriptorHeap*& dsDescriptorHeap,
    ID3D12Resource* constantBufferUploadHeaps[],
    UINT64(&fenceValue)[],
    HANDLE& fenceEvent,
    bool& Running); // release com ojects and clean up memory

void WaitForPreviousFrame(int& frameIndex,
    IDXGISwapChain3*& swapChain,
    ID3D12Fence* fence[],
    UINT64(&fenceValue)[],
    HANDLE& fenceEvent,
    bool& Running); // wait until gpu is finished with command list