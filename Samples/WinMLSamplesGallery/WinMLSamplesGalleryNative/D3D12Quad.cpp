#include "pch.h"
#include "stdafx.h"
#include "D3D12Quad.h"

inline std::string HrToString(HRESULT hr)
{
    return std::format("HRESULT of {:08X}", static_cast<UINT>(hr));
}

class HrException : public std::runtime_error
{
public:
    HrException(HRESULT hr) : std::runtime_error(HrToString(hr)), m_hr(hr) {}
    HRESULT Error() const { return m_hr; }
private:
    const HRESULT m_hr;
};

D3D12Quad::D3D12Quad(UINT width, UINT height, std::wstring name) :
    m_width(width),
    m_height(height),
    m_title(name),
    m_viewport(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height)),
    m_scissorRect(0, 0, static_cast<LONG>(width), static_cast<LONG>(height))
{
    m_aspectRatio = static_cast<float>(width) / static_cast<float>(height);
}

void D3D12Quad::OnInit()
{
    auto title = GetTitle();
    fileNames = {
        Win32Application::GetAssetPath(L"dog.jpg").c_str(),
        Win32Application::GetAssetPath(L"cat.jpg").c_str(),
        Win32Application::GetAssetPath(L"hummingbird.jpg").c_str()
    };
    fileIndex = 0;
    updateCounter = 0;
    LoadPipeline();
    LoadAssets();
    initializationSemaphore.release();
    copyTexture = false;
}

// Load the rendering pipeline dependencies.
void D3D12Quad::LoadPipeline()
{
    UINT dxgiFactoryFlags = 0;

    ComPtr<IDXGIFactory4> factory;
    ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory)));

    ComPtr<IDXGIAdapter1> hardwareAdapter;
    GetHardwareAdapter(factory.Get(), &hardwareAdapter);

    ThrowIfFailed(D3D12CreateDevice(
        hardwareAdapter.Get(),
        D3D_FEATURE_LEVEL_11_0,
        IID_PPV_ARGS(&m_device)
    ));

    // Describe and create the command queue.
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    ThrowIfFailed(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)));

    // Describe and create the swap chain.
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.BufferCount = FrameCount;
    swapChainDesc.Width = m_width;
    swapChainDesc.Height = m_height;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.SampleDesc.Count = 1;

    ComPtr<IDXGISwapChain1> swapChain;
    ThrowIfFailed(factory->CreateSwapChainForHwnd(
        m_commandQueue.Get(),        // Swap chain needs the queue so that it can force a flush on it.
        Win32Application::GetHwnd(),
        &swapChainDesc,
        nullptr,
        nullptr,
        &swapChain
    ));

    // This sample does not support fullscreen transitions.
    ThrowIfFailed(factory->MakeWindowAssociation(Win32Application::GetHwnd(), DXGI_MWA_NO_ALT_ENTER));

    ThrowIfFailed(swapChain.As(&m_swapChain));
    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

    CreateDescriptorHeaps();

    CreateFrameResources();

    ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator)));
}

void D3D12Quad::CreateDescriptorHeaps()
{
    // Describe and create a render target view (RTV) descriptor heap.
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
    rtvHeapDesc.NumDescriptors = FrameCount;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    ThrowIfFailed(m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap)));

    // Describe and create a shader resource view (SRV) heap for the texture.
    D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
    srvHeapDesc.NumDescriptors = 1;
    srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    ThrowIfFailed(m_device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_srvHeap)));

    m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
}

void D3D12Quad::CreateFrameResources()
{
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());

    // Create a RTV for each frame.
    for (UINT n = 0; n < FrameCount; n++)
    {
        ThrowIfFailed(m_swapChain->GetBuffer(n, IID_PPV_ARGS(&m_renderTargets[n])));
        m_device->CreateRenderTargetView(m_renderTargets[n].Get(), nullptr, rtvHandle);
        rtvHandle.Offset(1, m_rtvDescriptorSize);
    }
}

// Load the sample assets.
void D3D12Quad::LoadAssets()
{
    CreateRootSignature();

    CreatePipelineState();

    // Create the command list.
    ThrowIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator.Get(), m_pipelineState.Get(), IID_PPV_ARGS(&m_commandList)));

    CreateVertexBuffer();

    // Load Image Texture
    LoadImageTexture();

    // Create buffer that will hold the contents of the texture being drawn to the screen
    // in a resource dimension buffer. This will be used for inference in ORT
    CreateCurrentBuffer();

    CopyTextureIntoCurrentBuffer();

    // Close the command list and execute it to begin the initial GPU setup.
    ThrowIfFailed(m_commandList->Close());
    ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
    m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    CreateFence();
}

void D3D12Quad::CreateRootSignature()
{
    D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};

    // This is the highest version the sample supports. If CheckFeatureSupport succeeds, the HighestVersion returned will not be greater than this.
    featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

    if (FAILED(m_device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
    {
        featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
    }

    CD3DX12_DESCRIPTOR_RANGE1 ranges[1];
    ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);

    CD3DX12_ROOT_PARAMETER1 rootParameters[1];
    rootParameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_PIXEL);

    D3D12_STATIC_SAMPLER_DESC sampler = {};
    sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
    sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    sampler.MipLODBias = 0;
    sampler.MaxAnisotropy = 0;
    sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
    sampler.MinLOD = 0.0f;
    sampler.MaxLOD = D3D12_FLOAT32_MAX;
    sampler.ShaderRegister = 0;
    sampler.RegisterSpace = 0;
    sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
    rootSignatureDesc.Init_1_1(_countof(rootParameters), rootParameters, 1, &sampler, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> error;
    ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, featureData.HighestVersion, &signature, &error));
    ThrowIfFailed(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));
}

void D3D12Quad::CreatePipelineState()
{
    ComPtr<ID3DBlob> vertexShader;
    ComPtr<ID3DBlob> pixelShader;

#if defined(_DEBUG)
    // Enable better shader debugging with the graphics debugging tools.
    UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
    UINT compileFlags = 0;
#endif

    ThrowIfFailed(D3DCompileFromFile(Win32Application::GetAssetPath(L"shaders.hlsl").c_str(), nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &vertexShader, nullptr));
    ThrowIfFailed(D3DCompileFromFile(Win32Application::GetAssetPath(L"shaders.hlsl").c_str(), nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &pixelShader, nullptr));

    // Define the vertex input layout.
    D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    // Describe and create the graphics pipeline state object (PSO).
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
    psoDesc.pRootSignature = m_rootSignature.Get();
    psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());
    psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get());
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState.DepthEnable = FALSE;
    psoDesc.DepthStencilState.StencilEnable = FALSE;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.SampleDesc.Count = 1;
    ThrowIfFailed(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
}

void D3D12Quad::CreateVertexBuffer()
{
    // Define the geometry for a quad.
    Vertex quadVertices[] =
    {
        { { -1.0f, 1.0f * m_aspectRatio, 0.0f }, { 0.0f, 0.0f } }, //top left
        { { 1.0f, 1.0f * m_aspectRatio, 0.0f }, { 1.0f, 0.0f } }, //top right
        { { -1.0f, -1.0f * m_aspectRatio, 0.0f }, { 0.0f, 1.0f } }, //bottom left
        { { 1.0f, -1.0f * m_aspectRatio, 0.0f }, { 1.0f, 1.0f } }, //bottom right
    };

    const UINT vertexBufferSize = sizeof(quadVertices);

    // Note: using upload heaps to transfer static data like vert buffers is not 
    // recommended. Every time the GPU needs it, the upload heap will be marshalled 
    // over. Please read up on Default Heap usage. An upload heap is used here for 
    // code simplicity and because there are very few verts to actually transfer.
    auto heap_properties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    auto vb_resource_desc = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);
    ThrowIfFailed(m_device->CreateCommittedResource(
        &heap_properties,
        D3D12_HEAP_FLAG_NONE,
        &vb_resource_desc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_vertexBuffer)));

    // Copy the quad data to the vertex buffer.
    UINT8* pVertexDataBegin;
    CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
    ThrowIfFailed(m_vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
    memcpy(pVertexDataBegin, quadVertices, sizeof(quadVertices));
    m_vertexBuffer->Unmap(0, nullptr);

    // Initialize the vertex buffer view.
    m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
    m_vertexBufferView.StrideInBytes = sizeof(Vertex);
    m_vertexBufferView.SizeInBytes = vertexBufferSize;
}

void D3D12Quad::LoadImageTexture() {
    // Load the image from file
    D3D12_RESOURCE_DESC textureDesc;
    BYTE* imageData;
    int imageSize = LoadImageDataFromFile(&imageData, textureDesc, fileNames[fileIndex].c_str(), imageBytesPerRow);

    fileIndex++;
    if (fileIndex > 2)
        fileIndex = 0;

    // create a default heap where the upload heap will copy its contents into (contents being the texture)
    auto heap_properties_default = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    ThrowIfFailed(m_device->CreateCommittedResource(
        &heap_properties_default, // a default heap
        D3D12_HEAP_FLAG_NONE, // no flags
        &textureDesc, // the description of our texture
        D3D12_RESOURCE_STATE_COPY_DEST, // We will copy the texture from the upload heap to here, so we start it out in a copy dest state
        nullptr, // used for render targets and depth/stencil buffers
        IID_PPV_ARGS(&textureBuffer)));

    UINT64 textureUploadBufferSize;
    // this function gets the size an upload buffer needs to be to upload a texture to the gpu.
    // each row must be 256 byte aligned except for the last row, which can just be the size in bytes of the row
    // eg. textureUploadBufferSize = ((((width * numBytesPerPixel) + 255) & ~255) * (height - 1)) + (width * numBytesPerPixel);
    //textureUploadBufferSize = (((imageBytesPerRow + 255) & ~255) * (textureDesc.Height - 1)) + imageBytesPerRow;
    m_device->GetCopyableFootprints(&textureDesc, 0, 1, 0, nullptr, nullptr, nullptr, &textureUploadBufferSize);

    // now we create an upload heap to upload our texture to the GPU
    auto heap_properties_upload = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    auto tb_resource_desc = CD3DX12_RESOURCE_DESC::Buffer(textureUploadBufferSize);
    ThrowIfFailed(m_device->CreateCommittedResource(
        &heap_properties_upload, // upload heap
        D3D12_HEAP_FLAG_NONE, // no flags
        &tb_resource_desc, // resource description for a buffer (storing the image data in this heap just to copy to the default heap)
        D3D12_RESOURCE_STATE_GENERIC_READ, // We will copy the contents from this heap to the default heap above
        nullptr,
        IID_PPV_ARGS(&textureBufferUploadHeap)));

    // store vertex buffer in upload heap
    D3D12_SUBRESOURCE_DATA textureData = {};
    textureData.pData = &imageData[0]; // pointer to our image data
    textureData.RowPitch = imageBytesPerRow; // size of all our quad vertex data
    textureData.SlicePitch = imageBytesPerRow * textureDesc.Height; // also the size of our quad vertex data

    // Now we copy the upload buffer contents to the default heap
    UpdateSubresources(m_commandList.Get(), textureBuffer.Get(), textureBufferUploadHeap.Get(), 0, 0, 1, &textureData);

    // transition the texture default heap to a pixel shader resource (we will be sampling from this heap in the pixel shader to get the color of pixels)
    auto tb_transition = CD3DX12_RESOURCE_BARRIER::Transition(textureBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    m_commandList->ResourceBarrier(1, &tb_transition);

    // Describe and create a SRV for the texture.
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = textureDesc.Format;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    m_device->CreateShaderResourceView(textureBuffer.Get(), &srvDesc, m_srvHeap->GetCPUDescriptorHandleForHeapStart());
}

// Create buffer that will hold the contents of the texture being drawn to the screen
// in a resource dimension buffer. This will be used for inference in ORT
void D3D12Quad::CreateCurrentBuffer()
{
    auto desc = m_renderTargets[m_frameIndex]->GetDesc();
    D3D12_RESOURCE_DESC bufferDesc = {};
    bufferDesc.DepthOrArraySize = 1;
    bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    bufferDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
    bufferDesc.Height = 1;
    bufferDesc.Width = ((desc.Width* 4  + 255) & ~255) * desc.Height;
    bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    bufferDesc.MipLevels = 1;
    bufferDesc.SampleDesc.Count = 1;

    const CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_DEFAULT);
    auto some_hr = m_device->CreateCommittedResource(
        &heapProperties,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(&currentBuffer));
}

void D3D12Quad::CopyTextureIntoCurrentBuffer()
{
    const auto present_to_copy_src = CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_COPY_SOURCE);
    m_commandList->ResourceBarrier(1, &present_to_copy_src);

    auto desc = m_renderTargets[m_frameIndex]->GetDesc();
    UINT64 rowSizeInBytes, totalSizeInBytes;
    m_device->GetCopyableFootprints(&desc, 0, 1, 0, nullptr, nullptr, &rowSizeInBytes, &totalSizeInBytes);
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT bufferFootprint = {};
    bufferFootprint.Footprint.Width = static_cast<UINT>(desc.Width);
    bufferFootprint.Footprint.Height = desc.Height;
    bufferFootprint.Footprint.Depth = 1;
    bufferFootprint.Footprint.RowPitch = static_cast<UINT>((rowSizeInBytes + 255) & ~255);
    bufferFootprint.Footprint.Format = desc.Format;

    const CD3DX12_TEXTURE_COPY_LOCATION copyDest(currentBuffer.Get(), bufferFootprint);
    const CD3DX12_TEXTURE_COPY_LOCATION copySrc(m_renderTargets[m_frameIndex].Get(), 0);

    m_commandList->CopyTextureRegion(&copyDest, 0, 0, 0, &copySrc, nullptr);

    const auto copy_src_to_present = CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_PRESENT);
    m_commandList->ResourceBarrier(1, &copy_src_to_present);
}

// Create synchronization objects and wait until assets have been uploaded to the GPU.
void D3D12Quad::CreateFence()
{
    ThrowIfFailed(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
    m_fenceValue = 1;

    // Create an event handle to use for frame synchronization.
    m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (m_fenceEvent == nullptr)
    {
        ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
    }

    // Wait for the command list to execute; we are reusing the same command 
    // list in our main loop but for now, we just want to wait for setup to 
    // complete before continuing.
    WaitForPreviousFrame();
}

void D3D12Quad::Reset() {
    // Command list allocators can only be reset when the associated 
    // command lists have finished execution on the GPU; apps should use 
    // fences to determine GPU execution progress.
    ThrowIfFailed(m_commandAllocator->Reset());

    // However, when ExecuteCommandList() is called on a particular command 
    // list, that command list can then be reset at any time and must be before 
    // re-recording.
    ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), m_pipelineState.Get()));
}

// Update frame-based values.
void D3D12Quad::OnUpdate()
{
    justReset = false;
    if (copyTexture) {
        Reset();
        LoadImageTexture();
        justReset = true;
    }
}

// Render the scene.
void D3D12Quad::OnRender()
{
    // Record all the commands we need to render the scene into the command list.
    PopulateCommandList();

    // Execute the command list.
    ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
    m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    // Present the frame.
    ThrowIfFailed(m_swapChain->Present(1, 0));

    WaitForPreviousFrame();
}

void D3D12Quad::OnDestroy()
{
    // Ensure that the GPU is no longer referencing resources that are about to be
    // cleaned up by the destructor.
    WaitForPreviousFrame();

    CloseHandle(m_fenceEvent);
}

void D3D12Quad::PopulateCommandList()
{
    if (!justReset)
        Reset();

    // Set necessary state.
    m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());

    ID3D12DescriptorHeap* ppHeaps[] = { m_srvHeap.Get() };
    m_commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

    m_commandList->SetGraphicsRootDescriptorTable(0, m_srvHeap->GetGPUDescriptorHandleForHeapStart());

    m_commandList->RSSetViewports(1, &m_viewport);
    m_commandList->RSSetScissorRects(1, &m_scissorRect);

    // Indicate that the back buffer will be used as a render target.
    auto present_to_render = CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
    m_commandList->ResourceBarrier(1, &present_to_render);

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), m_frameIndex, m_rtvDescriptorSize);
    m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

    // Record commands.
    const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
    m_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
    m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    m_commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
    m_commandList->DrawInstanced(4, 1, 0, 0);

    // Indicate that the back buffer will now be used to present.
    auto render_to_present = CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
    m_commandList->ResourceBarrier(1, &render_to_present);

    if (copyTexture)
    {
        CopyTextureIntoCurrentBuffer();
        copyTexture = false;
    }

    ThrowIfFailed(m_commandList->Close());
}

void D3D12Quad::WaitForPreviousFrame()
{
    // Signal and increment the fence value.
    const UINT64 fence = m_fenceValue;
    ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), fence));
    m_fenceValue++;

    // Wait until the previous frame is finished.
    if (m_fence->GetCompletedValue() < fence)
    {
        ThrowIfFailed(m_fence->SetEventOnCompletion(fence, m_fenceEvent));
        WaitForSingleObject(m_fenceEvent, INFINITE);
    }

    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
}

// get the dxgi format equivilent of a wic format
DXGI_FORMAT D3D12Quad::GetDXGIFormatFromWICFormat(WICPixelFormatGUID wicFormatGUID)
{
    if (wicFormatGUID == GUID_WICPixelFormat128bppRGBAFloat) return DXGI_FORMAT_R32G32B32A32_FLOAT;
    else if (wicFormatGUID == GUID_WICPixelFormat64bppRGBAHalf) return DXGI_FORMAT_R16G16B16A16_FLOAT;
    else if (wicFormatGUID == GUID_WICPixelFormat64bppRGBA) return DXGI_FORMAT_R16G16B16A16_UNORM;
    else if (wicFormatGUID == GUID_WICPixelFormat32bppRGBA) return DXGI_FORMAT_R8G8B8A8_UNORM;
    else if (wicFormatGUID == GUID_WICPixelFormat32bppBGRA) return DXGI_FORMAT_B8G8R8A8_UNORM;
    else if (wicFormatGUID == GUID_WICPixelFormat32bppBGR) return DXGI_FORMAT_B8G8R8X8_UNORM;
    else if (wicFormatGUID == GUID_WICPixelFormat32bppRGBA1010102XR) return DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM;
    else if (wicFormatGUID == GUID_WICPixelFormat32bppRGBA1010102) return DXGI_FORMAT_R10G10B10A2_UNORM;
    else if (wicFormatGUID == GUID_WICPixelFormat16bppBGRA5551) return DXGI_FORMAT_B5G5R5A1_UNORM;
    else if (wicFormatGUID == GUID_WICPixelFormat16bppBGR565) return DXGI_FORMAT_B5G6R5_UNORM;
    else if (wicFormatGUID == GUID_WICPixelFormat32bppGrayFloat) return DXGI_FORMAT_R32_FLOAT;
    else if (wicFormatGUID == GUID_WICPixelFormat16bppGrayHalf) return DXGI_FORMAT_R16_FLOAT;
    else if (wicFormatGUID == GUID_WICPixelFormat16bppGray) return DXGI_FORMAT_R16_UNORM;
    else if (wicFormatGUID == GUID_WICPixelFormat8bppGray) return DXGI_FORMAT_R8_UNORM;
    else if (wicFormatGUID == GUID_WICPixelFormat8bppAlpha) return DXGI_FORMAT_A8_UNORM;
    else return DXGI_FORMAT_UNKNOWN;
}

// get a dxgi compatible wic format from another wic format
WICPixelFormatGUID D3D12Quad::GetConvertToWICFormat(WICPixelFormatGUID wicFormatGUID)
{
    if (wicFormatGUID == GUID_WICPixelFormatBlackWhite) return GUID_WICPixelFormat8bppGray;
    else if (wicFormatGUID == GUID_WICPixelFormat1bppIndexed) return GUID_WICPixelFormat32bppRGBA;
    else if (wicFormatGUID == GUID_WICPixelFormat2bppIndexed) return GUID_WICPixelFormat32bppRGBA;
    else if (wicFormatGUID == GUID_WICPixelFormat4bppIndexed) return GUID_WICPixelFormat32bppRGBA;
    else if (wicFormatGUID == GUID_WICPixelFormat8bppIndexed) return GUID_WICPixelFormat32bppRGBA;
    else if (wicFormatGUID == GUID_WICPixelFormat2bppGray) return GUID_WICPixelFormat8bppGray;
    else if (wicFormatGUID == GUID_WICPixelFormat4bppGray) return GUID_WICPixelFormat8bppGray;
    else if (wicFormatGUID == GUID_WICPixelFormat16bppGrayFixedPoint) return GUID_WICPixelFormat16bppGrayHalf;
    else if (wicFormatGUID == GUID_WICPixelFormat32bppGrayFixedPoint) return GUID_WICPixelFormat32bppGrayFloat;
    else if (wicFormatGUID == GUID_WICPixelFormat16bppBGR555) return GUID_WICPixelFormat16bppBGRA5551;
    else if (wicFormatGUID == GUID_WICPixelFormat32bppBGR101010) return GUID_WICPixelFormat32bppRGBA1010102;
    else if (wicFormatGUID == GUID_WICPixelFormat24bppBGR) return GUID_WICPixelFormat32bppRGBA;
    else if (wicFormatGUID == GUID_WICPixelFormat24bppRGB) return GUID_WICPixelFormat32bppRGBA;
    else if (wicFormatGUID == GUID_WICPixelFormat32bppPBGRA) return GUID_WICPixelFormat32bppRGBA;
    else if (wicFormatGUID == GUID_WICPixelFormat32bppPRGBA) return GUID_WICPixelFormat32bppRGBA;
    else if (wicFormatGUID == GUID_WICPixelFormat48bppRGB) return GUID_WICPixelFormat64bppRGBA;
    else if (wicFormatGUID == GUID_WICPixelFormat48bppBGR) return GUID_WICPixelFormat64bppRGBA;
    else if (wicFormatGUID == GUID_WICPixelFormat64bppBGRA) return GUID_WICPixelFormat64bppRGBA;
    else if (wicFormatGUID == GUID_WICPixelFormat64bppPRGBA) return GUID_WICPixelFormat64bppRGBA;
    else if (wicFormatGUID == GUID_WICPixelFormat64bppPBGRA) return GUID_WICPixelFormat64bppRGBA;
    else if (wicFormatGUID == GUID_WICPixelFormat48bppRGBFixedPoint) return GUID_WICPixelFormat64bppRGBAHalf;
    else if (wicFormatGUID == GUID_WICPixelFormat48bppBGRFixedPoint) return GUID_WICPixelFormat64bppRGBAHalf;
    else if (wicFormatGUID == GUID_WICPixelFormat64bppRGBAFixedPoint) return GUID_WICPixelFormat64bppRGBAHalf;
    else if (wicFormatGUID == GUID_WICPixelFormat64bppBGRAFixedPoint) return GUID_WICPixelFormat64bppRGBAHalf;
    else if (wicFormatGUID == GUID_WICPixelFormat64bppRGBFixedPoint) return GUID_WICPixelFormat64bppRGBAHalf;
    else if (wicFormatGUID == GUID_WICPixelFormat64bppRGBHalf) return GUID_WICPixelFormat64bppRGBAHalf;
    else if (wicFormatGUID == GUID_WICPixelFormat48bppRGBHalf) return GUID_WICPixelFormat64bppRGBAHalf;
    else if (wicFormatGUID == GUID_WICPixelFormat128bppPRGBAFloat) return GUID_WICPixelFormat128bppRGBAFloat;
    else if (wicFormatGUID == GUID_WICPixelFormat128bppRGBFloat) return GUID_WICPixelFormat128bppRGBAFloat;
    else if (wicFormatGUID == GUID_WICPixelFormat128bppRGBAFixedPoint) return GUID_WICPixelFormat128bppRGBAFloat;
    else if (wicFormatGUID == GUID_WICPixelFormat128bppRGBFixedPoint) return GUID_WICPixelFormat128bppRGBAFloat;
    else if (wicFormatGUID == GUID_WICPixelFormat32bppRGBE) return GUID_WICPixelFormat128bppRGBAFloat;
    else if (wicFormatGUID == GUID_WICPixelFormat32bppCMYK) return GUID_WICPixelFormat32bppRGBA;
    else if (wicFormatGUID == GUID_WICPixelFormat64bppCMYK) return GUID_WICPixelFormat64bppRGBA;
    else if (wicFormatGUID == GUID_WICPixelFormat40bppCMYKAlpha) return GUID_WICPixelFormat64bppRGBA;
    else if (wicFormatGUID == GUID_WICPixelFormat80bppCMYKAlpha) return GUID_WICPixelFormat64bppRGBA;

#if (_WIN32_WINNT >= _WIN32_WINNT_WIN8) || defined(_WIN7_PLATFORM_UPDATE)
    else if (wicFormatGUID == GUID_WICPixelFormat32bppRGB) return GUID_WICPixelFormat32bppRGBA;
    else if (wicFormatGUID == GUID_WICPixelFormat64bppRGB) return GUID_WICPixelFormat64bppRGBA;
    else if (wicFormatGUID == GUID_WICPixelFormat64bppPRGBAHalf) return GUID_WICPixelFormat64bppRGBAHalf;
#endif

    else return GUID_WICPixelFormatDontCare;
}

// get the number of bits per pixel for a dxgi format
int D3D12Quad::GetDXGIFormatBitsPerPixel(DXGI_FORMAT dxgiFormat)
{
    switch (dxgiFormat) {
    case DXGI_FORMAT_R32G32B32A32_FLOAT:
        return 128;
    case DXGI_FORMAT_R16G16B16A16_FLOAT:
        return 64;
    case DXGI_FORMAT_R16G16B16A16_UNORM:
        return 64;
    case DXGI_FORMAT_R8G8B8A8_UNORM:
        return 32;
    case DXGI_FORMAT_B8G8R8A8_UNORM:
        return 32;
    case DXGI_FORMAT_B8G8R8X8_UNORM:
        return 32;
    case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
        return 32;
    case DXGI_FORMAT_R10G10B10A2_UNORM:
        return 32;
    case DXGI_FORMAT_B5G5R5A1_UNORM:
        return 16;
    case DXGI_FORMAT_B5G6R5_UNORM:
        return 16;
    case DXGI_FORMAT_R32_FLOAT:
        return 32;
    case DXGI_FORMAT_R16_FLOAT:
        return 16;
    case DXGI_FORMAT_R16_UNORM:
        return 16;
    case DXGI_FORMAT_R8_UNORM:
        return 8;
    case DXGI_FORMAT_A8_UNORM:
        return 8;
    }
}

// load and decode image from file, returning the number of image bytes
int D3D12Quad::LoadImageDataFromFile(BYTE** imageData, D3D12_RESOURCE_DESC& resourceDescription, LPCWSTR filename, int& bytesPerRow)
{
    HRESULT hr;

    static IWICImagingFactory* wicFactory;

    // reset decoder, frame and converter since these will be different for each image we load
    IWICBitmapDecoder* wicDecoder = NULL;
    IWICBitmapFrameDecode* wicFrame = NULL;
    IWICFormatConverter* wicConverter = NULL;

    bool imageConverted = false;

    if (wicFactory == NULL)
    {
        // Initialize the COM library
        CoInitialize(NULL);

        // create the WIC factory
        hr = CoCreateInstance(
            CLSID_WICImagingFactory,
            NULL,
            CLSCTX_INPROC_SERVER,
            IID_PPV_ARGS(&wicFactory)
        );
        if (FAILED(hr)) return 0;
    }

    // load a decoder for the image
    hr = wicFactory->CreateDecoderFromFilename(
        filename,                        // Image we want to load in
        NULL,                            // This is a vendor ID, we do not prefer a specific one so set to null
        GENERIC_READ,                    // We want to read from this file
        WICDecodeMetadataCacheOnLoad,    // We will cache the metadata right away, rather than when needed, which might be unknown
        &wicDecoder                      // the wic decoder to be created
    );
    if (FAILED(hr)) return 0;

    // get image from decoder (this will decode the "frame")
    hr = wicDecoder->GetFrame(0, &wicFrame);
    if (FAILED(hr)) return 0;

    // get wic pixel format of image
    WICPixelFormatGUID pixelFormat;
    hr = wicFrame->GetPixelFormat(&pixelFormat);
    if (FAILED(hr)) return 0;

    // get size of image
    UINT textureWidth, textureHeight;
    hr = wicFrame->GetSize(&textureWidth, &textureHeight);
    if (FAILED(hr)) return 0;

    // convert wic pixel format to dxgi pixel format
    DXGI_FORMAT dxgiFormat = GetDXGIFormatFromWICFormat(pixelFormat);

    // if the format of the image is not a supported dxgi format, try to convert it
    if (dxgiFormat == DXGI_FORMAT_UNKNOWN)
    {
        // get a dxgi compatible wic format from the current image format
        WICPixelFormatGUID convertToPixelFormat = GetConvertToWICFormat(pixelFormat);

        // return if no dxgi compatible format was found
        if (convertToPixelFormat == GUID_WICPixelFormatDontCare) return 0;

        // set the dxgi format
        dxgiFormat = GetDXGIFormatFromWICFormat(convertToPixelFormat);

        // create the format converter
        hr = wicFactory->CreateFormatConverter(&wicConverter);
        if (FAILED(hr)) return 0;

        // make sure we can convert to the dxgi compatible format
        BOOL canConvert = FALSE;
        hr = wicConverter->CanConvert(pixelFormat, convertToPixelFormat, &canConvert);
        if (FAILED(hr) || !canConvert) return 0;

        // do the conversion (wicConverter will contain the converted image)
        hr = wicConverter->Initialize(wicFrame, convertToPixelFormat, WICBitmapDitherTypeErrorDiffusion, 0, 0, WICBitmapPaletteTypeCustom);
        if (FAILED(hr)) return 0;

        // this is so we know to get the image data from the wicConverter (otherwise we will get from wicFrame)
        imageConverted = true;
    }

    int bitsPerPixel = GetDXGIFormatBitsPerPixel(dxgiFormat); // number of bits per pixel
    bytesPerRow = (textureWidth * bitsPerPixel) / 8; // number of bytes in each row of the image data
    int imageSize = bytesPerRow * textureHeight; // total image size in bytes

    // allocate enough memory for the raw image data, and set imageData to point to that memory
    *imageData = (BYTE*)malloc(imageSize);

    // copy (decoded) raw image data into the newly allocated memory (imageData)
    if (imageConverted)
    {
        // if image format needed to be converted, the wic converter will contain the converted image
        hr = wicConverter->CopyPixels(0, bytesPerRow, imageSize, *imageData);
        if (FAILED(hr)) return 0;
    }
    else
    {
        // no need to convert, just copy data from the wic frame
        hr = wicFrame->CopyPixels(0, bytesPerRow, imageSize, *imageData);
        if (FAILED(hr)) return 0;
    }

    // now describe the texture with the information we have obtained from the image
    resourceDescription = {};
    resourceDescription.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    resourceDescription.Alignment = 0; // may be 0, 4KB, 64KB, or 4MB. 0 will let runtime decide between 64KB and 4MB (4MB for multi-sampled textures)
    resourceDescription.Width = textureWidth; // width of the texture
    resourceDescription.Height = textureHeight; // height of the texture
    resourceDescription.DepthOrArraySize = 1; // if 3d image, depth of 3d image. Otherwise an array of 1D or 2D textures (we only have one image, so we set 1)
    resourceDescription.MipLevels = 1; // Number of mipmaps. We are not generating mipmaps for this texture, so we have only one level
    resourceDescription.Format = dxgiFormat; // This is the dxgi format of the image (format of the pixels)
    resourceDescription.SampleDesc.Count = 1; // This is the number of samples per pixel, we just want 1 sample
    resourceDescription.SampleDesc.Quality = 0; // The quality level of the samples. Higher is better quality, but worse performance
    resourceDescription.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN; // The arrangement of the pixels. Setting to unknown lets the driver choose the most efficient one
    resourceDescription.Flags = D3D12_RESOURCE_FLAG_NONE; // no flags

    // return the size of the image.
    return imageSize;
}

void D3D12Quad::ThrowIfFailed(HRESULT hr)
{
    if (FAILED(hr))
    {
        throw HrException(hr);
    }
}

void D3D12Quad::GetHardwareAdapter(
    IDXGIFactory1* pFactory,
    IDXGIAdapter1** ppAdapter,
    bool requestHighPerformanceAdapter)
{
    *ppAdapter = nullptr;

    ComPtr<IDXGIAdapter1> adapter;

    ComPtr<IDXGIFactory6> factory6;
    if (SUCCEEDED(pFactory->QueryInterface(IID_PPV_ARGS(&factory6))))
    {
        for (
            UINT adapterIndex = 0;
            SUCCEEDED(factory6->EnumAdapterByGpuPreference(
                adapterIndex,
                requestHighPerformanceAdapter == true ? DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE : DXGI_GPU_PREFERENCE_UNSPECIFIED,
                IID_PPV_ARGS(&adapter)));
            ++adapterIndex)
        {
            DXGI_ADAPTER_DESC1 desc;
            adapter->GetDesc1(&desc);

            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) { continue; }

            // Check to see whether the adapter supports Direct3D 12, but don't create the
            // actual device yet.
            if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
            {
                break;
            }
        }
    }

    if (adapter.Get() == nullptr)
    {
        for (UINT adapterIndex = 0; SUCCEEDED(pFactory->EnumAdapters1(adapterIndex, &adapter)); ++adapterIndex)
        {
            DXGI_ADAPTER_DESC1 desc;
            adapter->GetDesc1(&desc);

            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            {
                // Don't select the Basic Render Driver adapter.
                // If you want a software adapter, pass in "/warp" on the command line.
                continue;
            }

            // Check to see whether the adapter supports Direct3D 12, but don't create the
            // actual device yet.
            if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
            {
                break;
            }
        }
    }

    *ppAdapter = adapter.Detach();
}

// Gets the buffer currently being drawn to the screen
// as a resource dimension buffer that will be passed into
// ORT for inference
ComPtr<ID3D12Resource> D3D12Quad::GetCurrentBuffer()
{
    return currentBuffer;
}

// Schedules the next image to be shown by setting the copyTexture
// variable to true. OnUpdate will catch this variable and load the
// next image then the image texture will be copied into a resource
// dimension buffer at the end of PopulateCommandLists
void D3D12Quad::ShowNextImage()
{
    copyTexture = true;
}

