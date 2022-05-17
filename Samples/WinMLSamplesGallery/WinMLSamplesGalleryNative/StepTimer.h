//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
//
//*********************************************************

#pragma once

#include <wrl.h>

namespace DX
{
	// Helper for animation and simulation timing.
	class StepTimer
	{
	public:
		// Constructor.
		StepTimer() :
			m_elapsedTicks(0),
			m_totalTicks(0),
			m_leftOverTicks(0),
			m_frameCount(0),
			m_framesPerSecond(0),
			m_framesThisSecond(0),
			m_qpcSecondCounter(0),
			m_isFixedTimeStep(false),
			m_targetElapsedTicks(TicksPerSecond / 60)
		{
			if (!QueryPerformanceFrequency(&m_qpcFrequency))
			{
				throw ref new Platform::FailureException();
			}

			if (!QueryPerformanceCounter(&m_qpcLastTime))
			{
				throw ref new Platform::FailureException();
			}

			// Initialize max delta to 1/10 of a second.
			m_qpcMaxDelta = m_qpcFrequency.QuadPart / 10;
		}

		// Get elapsed time since the previous Update call.
		uint64 GetElapsedTicks() const { return m_elapsedTicks; }
		double GetElapsedSeconds() const { return TicksToSeconds(m_elapsedTicks); }

		// Get total time since the start of the program.
		uint64 GetTotalTicks() const { return m_totalTicks; }
		double GetTotalSeconds() const { return TicksToSeconds(m_totalTicks); }

		// Get total number of updates since start of the program.
		uint32 GetFrameCount() const { return m_frameCount; }

		// Get the current framerate.
		uint32 GetFramesPerSecond() const { return m_framesPerSecond; }

		// Set whether to use fixed or variable timestep mode.
		void SetFixedTimeStep(bool isFixedTimestep) { m_isFixedTimeStep = isFixedTimestep; }

		// When in fixed timestep mode, set how often to call Update.
		void SetTargetElapsedTicks(uint64 targetElapsed) { m_targetElapsedTicks = targetElapsed; }
		void SetTargetElapsedSeconds(double targetElapsed) { m_targetElapsedTicks = SecondsToTicks(targetElapsed); }

		// Integer format represents time using 10,000,000 ticks per second.
		static const uint64 TicksPerSecond = 10000000;

		static double TicksToSeconds(uint64 ticks) { return (double)ticks / TicksPerSecond; }
		static uint64 SecondsToTicks(double seconds) { return (uint64)(seconds * TicksPerSecond); }

		// After an intentional timing discontinuity (for instance a blocking IO operation)
		// call this to avoid fixed timestep logic attempting a string of catch-up Update calls.
		void ResetElapsedTime()
		{
			if (!QueryPerformanceCounter(&m_qpcLastTime))
			{
				throw ref new Platform::FailureException();
			}

			m_leftOverTicks = 0;
			m_framesPerSecond = 0;
			m_framesThisSecond = 0;
			m_qpcSecondCounter = 0;
		}

		// Update timer state, calling the specified Update function the appropriate number of times.
		template<typename TUpdate>
		void Tick(const TUpdate& update)
		{
			// Query the current time.
			LARGE_INTEGER currentTime;

			if (!QueryPerformanceCounter(&currentTime))
			{
				throw ref new Platform::FailureException();
			}

			uint64 timeDelta = currentTime.QuadPart - m_qpcLastTime.QuadPart;

			m_qpcLastTime = currentTime;
			m_qpcSecondCounter += timeDelta;

			// Clamp excessively large time deltas (eg. after paused in the debugger).
			if (timeDelta > m_qpcMaxDelta)
			{
				timeDelta = m_qpcMaxDelta;
			}

			// Convert QPC units into our own canonical tick format. Cannot overflow due to the previous clamp.
			timeDelta *= TicksPerSecond;
			timeDelta /= m_qpcFrequency.QuadPart;

			uint32 lastFrameCount = m_frameCount;

			if (m_isFixedTimeStep)
			{
				// Fixed timestep update logic.

				// If we are running very close to the target elapsed time (within 1/4 ms) we just clamp
				// the clock to exactly match our target value. This prevents tiny and irrelvant errors
				// from accumulating over time. Without this clamping, a game that requested a 60 fps
				// fixed update, running with vsync enabled on a 59.94 NTSC display, would eventually
				// accumulate enough tiny errors that it would drop a frame. Better to just round such
				// small deviations down to zero, and leave things running smoothly.

				if (abs((int64)(timeDelta - m_targetElapsedTicks)) < TicksPerSecond / 4000)
				{
					timeDelta = m_targetElapsedTicks;
				}

				m_leftOverTicks += timeDelta;

				while (m_leftOverTicks >= m_targetElapsedTicks)
				{
					m_elapsedTicks = m_targetElapsedTicks;
					m_totalTicks += m_targetElapsedTicks;
					m_leftOverTicks -= m_targetElapsedTicks;
					m_frameCount++;

					update();
				}
			}
			else
			{
				// Variable timestep update logic.
				m_elapsedTicks = timeDelta;
				m_totalTicks += timeDelta;
				m_leftOverTicks = 0;
				m_frameCount++;

				update();
			}

			// Track the current framerate.
			if (m_frameCount != lastFrameCount)
			{
				m_framesThisSecond++;
			}

			if (m_qpcSecondCounter >= (uint64)m_qpcFrequency.QuadPart)
			{
				m_framesPerSecond = m_framesThisSecond;
				m_framesThisSecond = 0;
				m_qpcSecondCounter %= m_qpcFrequency.QuadPart;
			}
		}

	private:
		// Source timing data uses QPC units.
		LARGE_INTEGER m_qpcFrequency;
		LARGE_INTEGER m_qpcLastTime;
		uint64 m_qpcMaxDelta;

		// Derived timing data uses our own canonical tick format.
		uint64 m_elapsedTicks;
		uint64 m_totalTicks;
		uint64 m_leftOverTicks;

		// For tracking the framerate.
		uint32 m_frameCount;
		uint32 m_framesPerSecond;
		uint32 m_framesThisSecond;
		uint64 m_qpcSecondCounter;

		// For configuring fixed timestep mode.
		bool m_isFixedTimeStep;
		uint64 m_targetElapsedTicks;
	};
}
