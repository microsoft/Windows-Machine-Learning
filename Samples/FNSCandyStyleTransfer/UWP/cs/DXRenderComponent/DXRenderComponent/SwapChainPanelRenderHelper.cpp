#include "pch.h"
#include "SwapChainPanelRenderHelper.h"

using namespace Windows::Graphics::DirectX::Direct3D11;
using namespace winrt::Windows::Graphics::DirectX::Direct3D11;


void SetNameV(ID3D12Object *obj, const char *fmt, va_list args)
{
    char buf[256];
    int len = vsnprintf(buf, sizeof(buf), fmt, args);
    std::wstring temp(buf, buf + len);
    obj->SetName(temp.c_str());
}

static void SetName(ID3D12Object *obj, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    SetNameV(obj, fmt, args);
    va_end(args);
}

namespace winrt::DXRenderComponent::implementation
{
    SwapChainPanelRenderHelper::SwapChainPanelRenderHelper(
        Windows::UI::Xaml::Controls::SwapChainPanel panel, 
        Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice device,
        uint32_t width,
        uint32_t height) :
        _waitHandle(INVALID_HANDLE_VALUE)
    {
        // get the native panel interface
        _panelNative = panel.as<ISwapChainPanelNative>();

        // and now get the native dxgi device
        com_ptr<IDXGIDevice1> dxgiDevice;
        com_ptr<IDirect3DDxgiInterfaceAccess> dxgiInterfaceAccess = device.as<IDirect3DDxgiInterfaceAccess>();
        check_hresult(dxgiInterfaceAccess->GetInterface(IID_PPV_ARGS(dxgiDevice.put())));

        // get the DXGI adapter
        com_ptr<IDXGIAdapter> dxgiAdapter;
        check_hresult(dxgiDevice->GetAdapter(dxgiAdapter.put()));

        // create a d3d12 device from the dxgi adapter, we want a d3d12 command queue
        // since swapchains work better on 12
        check_hresult(D3D12CreateDevice(dxgiAdapter.get(), D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device1), m_device.put_void()));

        // create a command queue , since that is what swapchains need to create from
        D3D12_COMMAND_QUEUE_DESC queueDesc = {};
        queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        check_hresult(m_device->CreateCommandQueue(&queueDesc, winrt::guid_of<ID3D12CommandQueue>(), m_commandQueue.put_void()));

        SetName(m_commandQueue.get(), "swapchain_command_queue");

        // get the DXGI factory
        com_ptr<IDXGIFactory2> dxgiFactory;
        check_hresult(dxgiAdapter->GetParent(IID_PPV_ARGS(dxgiFactory.put())));

        // create the swapchain
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = { 0 };
        swapChainDesc.Width = width;
        swapChainDesc.Height = height;
        swapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;           // this is the most common swapchain format
        swapChainDesc.Stereo = false;
        swapChainDesc.SampleDesc.Count = 1;                          // don't use multi-sampling
        swapChainDesc.SampleDesc.Quality = 0;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.BufferCount = this->GetBufferCount();
        swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL
        swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT; //  ;

        // create swap chain by calling CreateSwapChainForComposition
        com_ptr<IDXGISwapChain1> swapChain1;
        check_hresult(dxgiFactory->CreateSwapChainForComposition(
            m_commandQueue.get(),
            &swapChainDesc,
            nullptr,        // allow on any display 
            swapChain1.put()
        ));

        _swapChain = swapChain1.as< IDXGISwapChain3>();
        check_hresult(_panelNative->SetSwapChain(_swapChain.get()));

        //_waitHandle = _swapChain->GetFrameLatencyWaitableObject();

        // Ensure that DXGI does not queue more than one frame at a time. This both reduces 
        // latency and ensures that the application will only render after each VSync, minimizing 
        // power consumption.
        //  
        check_hresult(_swapChain->SetMaximumFrameLatency(3));

        // now create the 11on12 device so we can use 11 surfaces for XAML
        IUnknown* queues = m_commandQueue.get();
        com_ptr<ID3D11DeviceContext> spContext;
        check_hresult(D3D11On12CreateDevice(
            m_device.get(),                     // input 12 device
            D3D11_CREATE_DEVICE_BGRA_SUPPORT,   // this is needed for Direct2D/interop rendering
            nullptr,                            // input feature levels
            0,
            &queues,                            // command queues
            1,
            0,                                  // GPU node (we are using 0)
            m_device11.put(),                   // output 11 device
            spContext.put(),                    // device context
            nullptr                             // output feature level
        ));

        m_device11on12 = m_device11.as<ID3D11On12Device>();
        spContext.as(m_deviceContext11);

        InitializeFences();
    }

    void SwapChainPanelRenderHelper::InitializeFences()
    {
        com_ptr<ID3D11Device5> device11_5;
        handle sharedFence;

        m_device11.as(device11_5);
        check_hresult(device11_5->CreateFence(0, D3D11_FENCE_FLAG_SHARED, IID_PPV_ARGS(_d3d11Fence.put())));

        check_hresult(_d3d11Fence->CreateSharedHandle(NULL, GENERIC_ALL, nullptr, sharedFence.put()));
        check_hresult(m_device->OpenSharedHandle(sharedFence.get(), IID_PPV_ARGS(_d3d12Fence.put())));
    }

    void SwapChainPanelRenderHelper::GPUSyncD3D11ToD3D12()
    {
        UINT64 currentFence = _fenceValue++;
        check_hresult(m_deviceContext11->Signal(_d3d11Fence.get(), currentFence));
        check_hresult(m_commandQueue->Wait(_d3d12Fence.get(), currentFence));
    }

    uint32_t SwapChainPanelRenderHelper::GetBufferCount()
    {
        return 3;
    }

    uint32_t SwapChainPanelRenderHelper::GetBackBufferIndex()
    {
        return _swapChain->GetCurrentBackBufferIndex();
    }

    Windows::Graphics::DirectX::Direct3D11::IDirect3DSurface SwapChainPanelRenderHelper::GetBackBuffer()
    {
        com_ptr<IDXGIResource> dxgiResource;
        com_ptr<IDXGISurface> dxgiSurface;
        com_ptr<ID3D12Resource> resource;
        com_ptr<ID3D11Resource> resource11;
        com_ptr<::IInspectable> inspectable;
        IDirect3DSurface surface;

        auto backBufferIndex = _swapChain->GetCurrentBackBufferIndex();

        // get the backbuffer, on 12 this will return us a ID3D12Resource, NOT a dxgi resource
        check_hresult(_swapChain->GetBuffer(backBufferIndex, IID_PPV_ARGS(resource.put())));

        SetName(resource.get(), "BackBuffer%d", backBufferIndex);

        D3D11_RESOURCE_FLAGS d3d11Flags = { D3D11_BIND_RENDER_TARGET };
        check_hresult(m_device11on12->CreateWrappedResource(
            resource.get(),
            &d3d11Flags, 
            D3D12_RESOURCE_STATE_COMMON, // D3D12_RESOURCE_STATE_RENDER_TARGET
            D3D12_RESOURCE_STATE_PRESENT,
            IID_PPV_ARGS(resource11.put())
        ));

        // create returns them already in the acquired state

        // get it as a dxgi surface
        dxgiSurface = resource11.as<IDXGISurface>();

        // return it as a winrt object
        check_hresult(CreateDirect3D11SurfaceFromDXGISurface(dxgiSurface.get(), inspectable.put()));
        surface = inspectable.as<::IDirect3DSurface>();
        return surface;
    }

    void SwapChainPanelRenderHelper::AcquireResource(Windows::Graphics::DirectX::Direct3D11::IDirect3DSurface surface)
    {
        // get the native dxgi device
        com_ptr<IDXGISurface> dxgiSurface;
        com_ptr<ID3D11Resource> resource11;
        com_ptr<IDirect3DDxgiInterfaceAccess> dxgiInterfaceAccess = surface.as<IDirect3DDxgiInterfaceAccess>();
        check_hresult(dxgiInterfaceAccess->GetInterface(IID_PPV_ARGS(dxgiSurface.put())));
        dxgiSurface.as(resource11);

        // acquire the wrapped resource
        ID3D11Resource * p = resource11.get();
        m_device11on12->AcquireWrappedResources(&p, 1);
    }

    void SwapChainPanelRenderHelper::ReleaseResource(Windows::Graphics::DirectX::Direct3D11::IDirect3DSurface surface)
    {
        // get the native dxgi device
        com_ptr<IDXGISurface> dxgiSurface;
        com_ptr<ID3D11Resource> resource11;
        com_ptr<IDirect3DDxgiInterfaceAccess> dxgiInterfaceAccess = surface.as<IDirect3DDxgiInterfaceAccess>();
        check_hresult(dxgiInterfaceAccess->GetInterface(IID_PPV_ARGS(dxgiSurface.put())));
        dxgiSurface.as(resource11);

        // release the wrapped resource
        ID3D11Resource * p = resource11.get();
        m_device11on12->ReleaseWrappedResources(&p, 1);

        // and flush the 11 cmd lists
        m_deviceContext11->Flush();
    }

    void SwapChainPanelRenderHelper::Present()
    {
        // flip the backbuffer and queue it for presentation
        check_hresult(_swapChain->Present(1, 0)); // DXGI_PRESENT_DO_NOT_WAIT
    }

    void SwapChainPanelRenderHelper::WaitForPresentReady()
    {
        if (_waitHandle != INVALID_HANDLE_VALUE)
        {
            DWORD result = WaitForSingleObjectEx(
                _waitHandle,
                1000, // 1 second timeout (shouldn't ever occur)
                true
            );
        }
    }

}