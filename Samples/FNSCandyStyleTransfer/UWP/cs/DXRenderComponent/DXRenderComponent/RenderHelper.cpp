#include "pch.h"
#include "RenderHelper.h"

using namespace Windows::Graphics::DirectX::Direct3D11;
using namespace winrt::Windows::Graphics::DirectX::Direct3D11;

namespace winrt::DXRenderComponent::implementation
{
    RenderHelper::RenderHelper(Windows::UI::Xaml::Media::Imaging::SurfaceImageSource source, IDirect3DDevice device)
    {
        _sourceNative = source.as<ISurfaceImageSourceNative>();
        com_ptr<IDXGIDevice> dxgiDevice;
        com_ptr<IDirect3DDxgiInterfaceAccess> dxgiInterfaceAccess = device.as<IDirect3DDxgiInterfaceAccess>();
        check_hresult(dxgiInterfaceAccess->GetInterface(IID_PPV_ARGS(dxgiDevice.put())));
        check_hresult(_sourceNative->SetDevice(dxgiDevice.get()));
    }

    void RenderHelper::EndDraw()
    {
        check_hresult(_sourceNative->EndDraw());
    }

    IDirect3DSurface RenderHelper::BeginDraw(int32_t width, int32_t heigth)
    {
        com_ptr<IDXGISurface> dxgiSurface;
        com_ptr<::IInspectable> inspectable;
        IDirect3DSurface surface;
        RECT updateRect = { 0, 0, width, heigth };
        POINT offset;
        check_hresult(_sourceNative->BeginDraw(updateRect, dxgiSurface.put(), &offset));
        check_hresult(CreateDirect3D11SurfaceFromDXGISurface(dxgiSurface.get(), inspectable.put()));
        surface = inspectable.as<::IDirect3DSurface>();
        return surface;
    }
}
