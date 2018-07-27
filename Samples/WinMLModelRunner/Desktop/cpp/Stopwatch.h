#pragma once

class Stopwatch
{
public:
    Stopwatch();

    void Click();

    // Time elapsed between last two clicks.
    inline double GetElapsedMilliseconds() const
    {
        return m_elapsedMilliseconds;
    }

private:
    LARGE_INTEGER m_lastClickTime;
    LARGE_INTEGER m_frequency;
    double m_elapsedMilliseconds = 0.0;
};