#pragma once
#include "StreamEffect.g.h"


namespace winrt::WinMLSamplesGalleryNative::implementation
{
    struct StreamEffect : StreamEffectT<StreamEffect>
    {
        StreamEffect() = default;

        static void LaunchNewWindow();
    };
}
namespace winrt::WinMLSamplesGalleryNative::factory_implementation
{
    struct StreamEffect : StreamEffectT<StreamEffect, implementation::StreamEffect>
    {
    };
}
