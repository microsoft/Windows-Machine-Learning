#include "pch.h"
#include "StyleTransferEffectNotifier.h"
#include "StyleTransferEffectNotifier.g.cpp"


namespace winrt::StyleTransferEffectCpp::implementation
{
    void StyleTransferEffectNotifier::SetFrameRate(float value)
    {
        m_balance = value;
        m_frameRateUpdatedEvent(*this, m_balance);
    }
    winrt::event_token StyleTransferEffectNotifier::FrameRateUpdated(Windows::Foundation::EventHandler<float> const& handler)
    {
        return m_frameRateUpdatedEvent.add(handler);
    }
    void StyleTransferEffectNotifier::FrameRateUpdated(winrt::event_token const& token) noexcept
    {
        m_frameRateUpdatedEvent.remove(token);
    }
}
