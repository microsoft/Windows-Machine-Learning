#include <Windows.h>
#include "Stopwatch.h"

Stopwatch::Stopwatch()
{
    QueryPerformanceFrequency(&m_frequency);
}

void Stopwatch::Click()
{
    LARGE_INTEGER currentTime;
    QueryPerformanceCounter(&currentTime);
    auto delta = static_cast<double>(currentTime.QuadPart - m_lastClickTime.QuadPart);
    m_elapsedMilliseconds = delta / (static_cast<double>(m_frequency.QuadPart) / 1000.0);
    m_lastClickTime = currentTime;
}
