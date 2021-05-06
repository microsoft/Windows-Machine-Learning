using System;
using System.Threading.Tasks;
using Windows.Graphics.Imaging;
using Windows.Media;

namespace classifierMLNETModel
{
    public class Helper
    {
        private const int IMAGE_HEIGHT = 224;
        VideoFrame cropped_vf = null;
        public async Task<VideoFrame> CropAndDisplayInputImageAsync(VideoFrame inputVideoFrame)
        {
            bool useDX = inputVideoFrame.SoftwareBitmap == null;

            BitmapBounds cropBounds = new BitmapBounds();
            uint h = IMAGE_HEIGHT;
            uint w = IMAGE_HEIGHT;
            var frameHeight = useDX ? inputVideoFrame.Direct3DSurface.Description.Height : inputVideoFrame.SoftwareBitmap.PixelHeight;
            var frameWidth = useDX ? inputVideoFrame.Direct3DSurface.Description.Width : inputVideoFrame.SoftwareBitmap.PixelWidth;

            var requiredAR = ((float)IMAGE_HEIGHT / IMAGE_HEIGHT);
            w = Math.Min((uint)(requiredAR * frameHeight), (uint)frameWidth);
            h = Math.Min((uint)(frameWidth / requiredAR), (uint)frameHeight);
            cropBounds.X = (uint)((frameWidth - w) / 2);
            cropBounds.Y = 0;
            cropBounds.Width = w;
            cropBounds.Height = h;

            cropped_vf = new VideoFrame(BitmapPixelFormat.Bgra8, IMAGE_HEIGHT, IMAGE_HEIGHT, BitmapAlphaMode.Ignore);

            await inputVideoFrame.CopyToAsync(cropped_vf, cropBounds, null);
            return cropped_vf;
        }
    }
}
