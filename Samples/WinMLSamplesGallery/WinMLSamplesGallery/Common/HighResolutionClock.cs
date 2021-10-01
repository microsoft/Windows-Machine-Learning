using System;
using System.Runtime.InteropServices;

namespace WinMLSamplesGallery.Samples
{
    public static class HighResolutionClock {

        [DllImport("Kernel32.dll", CallingConvention = CallingConvention.Winapi)]
        private static extern void GetSystemTimePreciseAsFileTime(out long filetime);

        public static long UtcNow() {
            long filetime;
            GetSystemTimePreciseAsFileTime(out filetime);
            return filetime;
        }

        public static float DurationInMs(long start, long stop) {
            var duration = DateTime.FromFileTimeUtc(stop) - DateTime.FromFileTimeUtc(start);
            return (float)duration.Ticks / 10 / 1000;
        }
    }
}
