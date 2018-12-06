#pragma once

#include "RenderHelper.g.h"

namespace winrt::DXRenderComponent::implementation
{
    struct RenderHelper : RenderHelperT<RenderHelper>
    {
        RenderHelper() = delete;

        RenderHelper(Windows::UI::Xaml::Media::Imaging::SurfaceImageSource source, Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice device);

        Windows::Graphics::DirectX::Direct3D11::IDirect3DSurface BeginDraw(uint32_t width, uint32_t heigth);
        void EndDraw();

    private:
        com_ptr<ISurfaceImageSourceNative> _sourceNative;
        int _width;
        int _heigth;
    };
}

namespace winrt::DXRenderComponent::factory_implementation
{
    struct RenderHelper : RenderHelperT<RenderHelper, implementation::RenderHelper>
    {
    };
}
