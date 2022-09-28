#pragma once
#include "DXResourceBinding.g.h"

namespace winrt::WinMLSamplesGalleryNative::implementation
{
    struct DXResourceBinding : DXResourceBindingT<DXResourceBinding>
    {
        DXResourceBinding() = default;
        static void LaunchWindow(winrt::hstring appPath);
        static winrt::com_array<float> EvalORT();
        static void CloseWindow();
    };
}
namespace winrt::WinMLSamplesGalleryNative::factory_implementation
{
    struct DXResourceBinding : DXResourceBindingT<DXResourceBinding, implementation::DXResourceBinding>
    {
    };
}

