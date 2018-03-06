using System;
using System.Diagnostics;
using System.Threading;
using Windows.Graphics.Imaging;
using Windows.UI.Core;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Media.Imaging;

namespace StyleTransfer
{
    public sealed class FrameRenderer
    {
        private Image _imageElement;
        private SoftwareBitmap _backBuffer;
        private bool _taskRunning = false;

        public FrameRenderer(Image imageElement)
        {
            _imageElement = imageElement;
            _imageElement.Source = new SoftwareBitmapSource();
        }

        public void RenderFrame(SoftwareBitmap softwareBitmap)
        {
            if (softwareBitmap != null)
            {
                // Swap the processed frame to _backBuffer and trigger UI thread to render it
                softwareBitmap = Interlocked.Exchange(ref _backBuffer, softwareBitmap);

                // UI thread always reset _backBuffer before using it.  Unused bitmap should be disposed.
                softwareBitmap?.Dispose();

                // Changes to xaml ImageElement must happen in UI thread through Dispatcher
                var task = _imageElement.Dispatcher.RunAsync(CoreDispatcherPriority.Normal,
                    async () =>
                    {
                        // Don't let two copies of this task run at the same time.
                        if (_taskRunning)
                        {
                            return;
                        }
                        _taskRunning = true;
                        try
                        {
                            // Keep draining frames from the backbuffer until the backbuffer is empty.
                            SoftwareBitmap latestBitmap;
                            while ((latestBitmap = Interlocked.Exchange(ref _backBuffer, null)) != null)
                            {
                                if (_imageElement.MaxHeight != latestBitmap.PixelHeight)
                                    _imageElement.MaxHeight = latestBitmap.PixelHeight;

                                if (_imageElement.MaxWidth != latestBitmap.PixelWidth)
                                    _imageElement.MaxWidth = latestBitmap.PixelWidth;

                                var imageSource = (SoftwareBitmapSource)_imageElement.Source;
                                await imageSource.SetBitmapAsync(latestBitmap);
                                latestBitmap.Dispose();
                            }
                        }
                        catch (Exception ex)
                        {
                            Debug.WriteLine(ex.Message);
                        }
                        _taskRunning = false;
                    });
            }
        }
    }
}
