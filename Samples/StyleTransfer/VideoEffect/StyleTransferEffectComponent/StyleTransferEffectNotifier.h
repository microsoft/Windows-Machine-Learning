#pragma once
#include "StyleTransferEffectNotifier.g.h"

namespace winrt::StyleTransferEffectComponent::implementation
{
    struct StyleTransferEffectNotifier : StyleTransferEffectNotifierT<StyleTransferEffectNotifier>
    {
        StyleTransferEffectNotifier() = default;
        void SetFrameRate(float value);
        winrt::event_token FrameRateUpdated(Windows::Foundation::EventHandler<float> const& handler);
        void FrameRateUpdated(winrt::event_token const& token) noexcept;
    private:
        winrt::event<Windows::Foundation::EventHandler<float>> _frameRateUpdatedEvent;
        float _balance{ 0.f };
    };
}

namespace winrt::StyleTransferEffectComponent::factory_implementation
{
    struct StyleTransferEffectNotifier : StyleTransferEffectNotifierT<StyleTransferEffectNotifier, implementation::StyleTransferEffectNotifier>
    {
    };
}
