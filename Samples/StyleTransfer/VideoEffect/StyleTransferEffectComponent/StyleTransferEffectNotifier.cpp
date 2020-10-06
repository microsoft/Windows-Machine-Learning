#include "pch.h"
#include "StyleTransferEffectNotifier.h"
#include "StyleTransferEffectNotifier.g.cpp"


namespace winrt::StyleTransferEffectComponent::implementation
{
    void StyleTransferEffectNotifier::SetFrameRate(float value)
    {
        _balance = value;
        _frameRateUpdatedEvent(*this, _balance);
    }
    winrt::event_token StyleTransferEffectNotifier::FrameRateUpdated(Windows::Foundation::EventHandler<float> const& handler)
    {
        return _frameRateUpdatedEvent.add(handler);
    }
    void StyleTransferEffectNotifier::FrameRateUpdated(winrt::event_token const& token) noexcept
    {
        _frameRateUpdatedEvent.remove(token);
    }
}
