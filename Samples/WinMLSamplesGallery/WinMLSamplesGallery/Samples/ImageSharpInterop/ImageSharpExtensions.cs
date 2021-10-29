using System;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;

using SixLabors.ImageSharp;
using SixLabors.ImageSharp.Processing;
using SixLabors.ImageSharp.PixelFormats;
using SixLabors.ImageSharp.Advanced;
using SixLabors.ImageSharp.Memory;

using Microsoft.AI.MachineLearning;
using Windows.Graphics.Imaging;

namespace ImageSharpExtensionMethods
{
    public static class ImageExtensions
    {
        public static Windows.Storage.Streams.IBuffer AsBuffer<TPixel>(this Image<TPixel> img) where TPixel : unmanaged, IPixel<TPixel>
        {
            var memoryGroup = img.GetPixelMemoryGroup();
            var memory = memoryGroup.ToArray()[0];
            var pixelData = System.Runtime.InteropServices.MemoryMarshal.AsBytes(memory.Span).ToArray(); // Can we get rid of this?
            var buffer = pixelData.AsBuffer();
            return buffer;
        }

        public static ITensor AsTensor<TPixel>(this Image<TPixel> img) where TPixel : unmanaged, IPixel<TPixel>
        {
            var buffer = img.AsBuffer();
            var shape = new long[] { 1, buffer.Length };
            var tensor = TensorUInt8Bit.CreateFromBuffer(shape, buffer);
            return tensor;
        }


        public static SoftwareBitmap AsSoftwareBitmap<TPixel>(this Image<TPixel> img) where TPixel : unmanaged, IPixel<TPixel>
        {
            var buffer = img.AsBuffer();
            var format = BitmapPixelFormat.Unknown;
            if (typeof(TPixel) == typeof(Bgra32))
            {
                format = BitmapPixelFormat.Bgra8;
            }

            var softwareBitmap = SoftwareBitmap.CreateCopyFromBuffer(buffer, BitmapPixelFormat.Bgra8, img.Width, img.Height);
            return softwareBitmap;
        }
    }
}
