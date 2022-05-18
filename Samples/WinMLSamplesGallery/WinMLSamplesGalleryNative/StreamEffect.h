#pragma once
#include "StreamEffect.g.h"


namespace winrt::WinMLSamplesGalleryNative::implementation
{
    struct StreamEffect : StreamEffectT<StreamEffect>
    {
        StreamEffect() = default;

        static void LaunchNewWindow(winrt::hstring modelPath);

        static void ShutDownWindow();
        static int32_t CreateInferenceWindow();
    };
}
namespace winrt::WinMLSamplesGalleryNative::factory_implementation
{
    struct StreamEffect : StreamEffectT<StreamEffect, implementation::StreamEffect>
    {
    };
}
