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
/*
  // Will need this for 0 copy interop
    public class PinnedBuffer : Windows.Storage.Streams.IBuffer, IBufferByteAccess


    {
        GCHandle handle_;
        IntPtr ptr_;
        uint num_bytes_;

        public PinnedBuffer(byte[] array) {
            num_bytes_ = (uint)array.Length;
            handle_ = GCHandle.Alloc(array, GCHandleType.Pinned);
            ptr_ = GCHandle.ToIntPtr(handle_);
        }

        ~PinnedBuffer() {
            handle_.Free();
        }

        public uint Capacity => num_bytes_;

        public uint Length
        {
            get => num_bytes_;
            set
            {
                num_bytes_ = value;
            }
        }

#pragma warning disable CA1416 // Validate platform compatibility
        public TensorFloat ToTensorFloat()
        {
            return TensorFloat.CreateFromBuffer(new long[] { num_bytes_ }, this);
        }

        public unsafe void Buffer(out byte* pByte)
        {
            pByte = (byte*)ptr_;
        }
#pragma warning restore CA1416 // Validate platform compatibility
    }
 */