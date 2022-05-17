//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
//
//*********************************************************

#pragma once
#include "pch.h"
#include <concrt.h>
#include <windows.ui.xaml.h>

namespace DirectXPanels
{
    // Base class for a SwapChainPanel-based DirectX rendering surface to be used in XAML apps.
    //[Windows::Foundation::Metadata::WebHostHidden]
    class DirectXPanelBase : public Windows::UI::Xaml::Controls::SwapChainPanel
    {
    protected private:
        DirectXPanelBase();

        virtual void CreateDeviceIndependentResources();
        virtual void CreateDeviceResources();
        virtual void CreateSizeDependentResources();

        virtual void OnDeviceLost();
        virtual void OnSizeChanged(Platform::Object^ sender, Windows::UI::Xaml::SizeChangedEventArgs^ e);
        virtual void OnCompositionScaleChanged(Windows::UI::Xaml::Controls::SwapChainPanel^ sender, Platform::Object^ args);
        virtual void OnSuspending(Platform::Object^ sender, Windows::ApplicationModel::SuspendingEventArgs^ e);
        virtual void OnResuming(Platform::Object^ sender, Platform::Object^ args) { };

        virtual void Render() { };
        virtual void Present();

        Microsoft::WRL::ComPtr<ID3D11Device1>                               m_d3dDevice;
        Microsoft::WRL::ComPtr<ID3D11DeviceContext1>                        m_d3dContext;
        Microsoft::WRL::ComPtr<IDXGISwapChain2>                             m_swapChain;

        Microsoft::WRL::ComPtr<ID2D1Factory2>                               m_d2dFactory;
        Microsoft::WRL::ComPtr<ID2D1Device>                                 m_d2dDevice;
        Microsoft::WRL::ComPtr<ID2D1DeviceContext>                          m_d2dContext;
        Microsoft::WRL::ComPtr<ID2D1Bitmap1>                                m_d2dTargetBitmap;

        D2D1_COLOR_F                                                        m_backgroundColor;
        DXGI_ALPHA_MODE                                                     m_alphaMode;

        bool                                                                m_loadingComplete;

        Concurrency::critical_section                                       m_criticalSection;

        float                                                               m_renderTargetHeight;
        float                                                               m_renderTargetWidth;

        float                                                               m_compositionScaleX;
        float                                                               m_compositionScaleY;

        float                                                               m_height;
        float                                                               m_width;

    };
}