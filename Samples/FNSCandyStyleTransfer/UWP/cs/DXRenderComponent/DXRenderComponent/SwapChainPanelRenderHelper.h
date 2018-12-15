#pragma once

#include "SwapChainPanelRenderHelper.g.h"

namespace winrt::DXRenderComponent::implementation
{
    struct SwapChainPanelRenderHelper : SwapChainPanelRenderHelperT<SwapChainPanelRenderHelper>
    {
        SwapChainPanelRenderHelper() = delete;

        SwapChainPanelRenderHelper(
            Windows::UI::Xaml::Controls::SwapChainPanel panel, 
            Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice device,
            uint32_t width,
            uint32_t heigth);

        Windows::Graphics::DirectX::Direct3D11::IDirect3DSurface GetBackBuffer();
        uint32_t GetBackBufferIndex();
        uint32_t GetBufferCount();
        void Present();
        void WaitForPresentReady();

    private:
        com_ptr<ISwapChainPanelNative>  _panelNative;
        com_ptr<IDXGISwapChain3>        _swapChain;
        com_ptr<ID3D12Device1>          m_device;
        com_ptr<ID3D11On12Device>       m_device11on12;
        com_ptr<ID3D11Device>           m_device11;
        com_ptr<ID3D12CommandQueue>     m_commandQueue;
        HANDLE                          _waitHandle;
    };
}

namespace winrt::DXRenderComponent::factory_implementation
{
    struct SwapChainPanelRenderHelper : SwapChainPanelRenderHelperT<SwapChainPanelRenderHelper, implementation::SwapChainPanelRenderHelper>
    {
    };
}
