#pragma once
#include "StyleTransferEffectNotifier.g.h"


namespace winrt::StyleTransferEffectCpp::implementation
{
    struct StyleTransferEffectNotifier : StyleTransferEffectNotifierT<StyleTransferEffectNotifier>
    {
        StyleTransferEffectNotifier() = default;
        void SetFrameRate(float value);
        winrt::event_token FrameRateUpdated(Windows::Foundation::EventHandler<float> const& handler);
        void FrameRateUpdated(winrt::event_token const& token) noexcept;
    private:
        winrt::event<Windows::Foundation::EventHandler<float>> m_frameRateUpdatedEvent;
        float m_balance{ 0.f };
    };
}

namespace winrt::StyleTransferEffectCpp::factory_implementation
{
    struct StyleTransferEffectNotifier : StyleTransferEffectNotifierT<StyleTransferEffectNotifier, implementation::StyleTransferEffectNotifier>
    {
    };
}
