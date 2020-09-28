// Copyright (C) Microsoft Corporation. All rights reserved.
using System;
using System.Threading;
using System.Threading.Tasks;
using Windows.Graphics.DirectX;
using Windows.Media;
using Windows.Media.Core;
using Windows.Media.Playback;
using Windows.Storage;
using Microsoft.Graphics.Canvas;
using Windows.Graphics.Imaging;
using GalaSoft.MvvmLight.Threading;
using Windows.Media;
namespace FrameSourceHelper_UWP
{
    /// <summary>
    /// MediaPlayer backed FrameSource
    /// </summary>
    public sealed class MediaPlayerFrameSource : IDisposable
    {
        private MediaPlayer m_mediaPlayer = null;

        private VideoFrame m_videoFrame;
        private VideoFrame m_stagingVideoFrame = null;
        private EventWaitHandle m_frameSourceReadyEvent = new EventWaitHandle(false, EventResetMode.ManualReset);
        private EventHandler<string> m_failureHandler;
        public uint FrameWidth { get; private set; }
        public uint FrameHeight { get; private set; }

        /// <summary>
        /// Static factory
        /// </summary>
        /// <returns></returns>
        public static async Task<MediaPlayerFrameSource> CreateFromStorageFileAsyncTask(
            StorageFile storageFile)
        {
            var result = new MediaPlayerFrameSource()
            {
                m_mediaPlayer = new MediaPlayer()
                {
                    Source = MediaSource.CreateFromStorageFile(storageFile),
                    IsVideoFrameServerEnabled = true,
                    RealTimePlayback = true,
                    IsMuted = true,
                    IsLoopingEnabled = true
                }
            };

            result.m_mediaPlayer.CommandManager.IsEnabled = false;
            result.m_mediaPlayer.MediaOpened += result.MediaPlayer_MediaOpened;
            result.m_mediaPlayer.MediaEnded += result.MediaPlayer_MediaEnded;
            result.m_mediaPlayer.MediaFailed += result.MediaPlayer_MediaFailed;

            await Task.Run(() => result.m_frameSourceReadyEvent.WaitOne());

            return result;
        }

        /// <summary>
        /// Private constructor called by static factory
        /// </summary>
        /// <returns></returns>
        private MediaPlayerFrameSource()
        {
        }

        /// <summary>
        /// Start frame playback
        /// </summary>
        public Task StartAsync()
        {
            m_mediaPlayer.Play();

            // Async not needed, return success
            return Task.FromResult(true);
        }

        public Task StopAsync()
        {
            m_mediaPlayer.Pause();

            // Async not needed, return success
            return Task.FromResult(true);
        }

        public event EventHandler<VideoFrame> FrameArrived;
        public event EventHandler StreamEnded;

        /// <summary>
        /// MediaPlayer.MediaOpened event handler. Completes frame source initialization
        /// by allocating frame buffer and registering for VideoFrameAvailable event
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="args"></param>
        private void MediaPlayer_MediaOpened(MediaPlayer sender, object args)
        {
            // Retrieve media source resolution
            FrameWidth = m_mediaPlayer.PlaybackSession.NaturalVideoWidth;
            FrameHeight = m_mediaPlayer.PlaybackSession.NaturalVideoHeight;

            // Allocate and register for frames
            m_videoFrame = VideoFrame.CreateAsDirect3D11SurfaceBacked(
                            DirectXPixelFormat.B8G8R8A8UIntNormalized,
                            (int)FrameWidth,
                            (int)FrameHeight);

            m_mediaPlayer.VideoFrameAvailable += MediaPlayer_VideoFrameAvailable;

            m_frameSourceReadyEvent.Set();
        }

        /// <summary>
        /// MediaFrameReader.FrameArrived callback. Extracts VideoFrame and timestamp and forwards event
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="args"></param>
        private async void MediaPlayer_VideoFrameAvailable(MediaPlayer sender, object args)
        {
            // m_mediaPlayer.CopyFrameToVideoSurface(m_videoFrame.Direct3DSurface);
            
            // m_videoFrame.SystemRelativeTime = m_mediaPlayer.PlaybackSession.Position;
            // FrameArrived?.Invoke(this, m_videoFrame);
            CanvasDevice canvasDevice = CanvasDevice.GetSharedDevice();
            await DispatcherHelper.RunAsync(async () =>
            {
                SoftwareBitmap softwareBitmapImg;
                SoftwareBitmap frameServerDest = new SoftwareBitmap(BitmapPixelFormat.Rgba8, 100, 100, BitmapAlphaMode.Premultiplied);

                using (CanvasBitmap canvasBitmap = CanvasBitmap.CreateFromSoftwareBitmap(canvasDevice, frameServerDest))
                {
                    sender.CopyFrameToVideoSurface(canvasBitmap);

                    softwareBitmapImg = await SoftwareBitmap.CreateCopyFromSurfaceAsync(canvasBitmap);

                }
                m_videoFrame = VideoFrame.CreateWithSoftwareBitmap(softwareBitmapImg);
                m_videoFrame.SystemRelativeTime = m_mediaPlayer.PlaybackSession.Position;
                FrameArrived?.Invoke(this, m_videoFrame);
            });
        }

        /// <summary>
        /// MediaPlayer.MediaEnded event callback. Forwards event
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="args"></param>
        private void MediaPlayer_MediaEnded(MediaPlayer sender, object args)
        {
            StreamEnded?.Invoke(this, null);
        }

        /// <summary>
        /// Triggered when the MediaPlayer fails. Forwards the event
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="args"></param>
        private void MediaPlayer_MediaFailed(MediaPlayer sender, MediaPlayerFailedEventArgs args)
        {
            m_failureHandler?.Invoke(this, $"Error: {args.Error} : {args.ErrorMessage}");
        }

        /// <summary>
        /// Dispose method implementation
        /// </summary>
        public void Dispose()
        {
            m_mediaPlayer?.Pause();
            m_mediaPlayer?.Dispose();
        }
    }
}