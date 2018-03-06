using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.Foundation;
using Windows.Graphics.Imaging;
using Windows.Media;
using Windows.Storage;
using Windows.Storage.Pickers;
using Windows.Storage.Streams;

namespace StyleTransfer
{
    public sealed class ImageHelper
    {
        /// <summary>
        /// Pass the nput frame to a frame renderer and ensure proper image format is used
        /// </summary>
        /// <param name="inputVideoFrame"></param>
        /// <param name="useDX"></param>
        /// <returns></returns>
        public static IAsyncAction RenderFrameAsync(FrameRenderer frameRenderer, VideoFrame inputVideoFrame)
        {
            return AsyncInfo.Run(async (token) =>
            {
                bool useDX = inputVideoFrame.SoftwareBitmap == null;
                if (frameRenderer == null)
                {
                    throw (new InvalidOperationException("FrameRenderer is null"));
                }

                SoftwareBitmap softwareBitmap = null;
                if (useDX)
                {
                    softwareBitmap = await SoftwareBitmap.CreateCopyFromSurfaceAsync(inputVideoFrame.Direct3DSurface);
                    softwareBitmap = SoftwareBitmap.Convert(softwareBitmap, BitmapPixelFormat.Bgra8, BitmapAlphaMode.Premultiplied);
                }
                else
                {
                    softwareBitmap = new SoftwareBitmap(
                        inputVideoFrame.SoftwareBitmap.BitmapPixelFormat,
                        inputVideoFrame.SoftwareBitmap.PixelWidth,
                        inputVideoFrame.SoftwareBitmap.PixelHeight,
                        inputVideoFrame.SoftwareBitmap.BitmapAlphaMode);
                    inputVideoFrame.SoftwareBitmap.CopyTo(softwareBitmap);
                }

                frameRenderer.RenderFrame(softwareBitmap);
            });
        }

        /// <summary>
        /// Launch file picker for user to select a picture file and return a VideoFrame
        /// </summary>
        /// <returns>VideoFrame instanciated from the selected image file</returns>
        public static IAsyncOperation<VideoFrame> LoadVideoFrameFromFilePickedAsync()
        {
            return AsyncInfo.Run(async (token) =>
            {
                // Trigger file picker to select an image file
                FileOpenPicker fileOpenPicker = new FileOpenPicker();
                fileOpenPicker.SuggestedStartLocation = PickerLocationId.PicturesLibrary;
                fileOpenPicker.FileTypeFilter.Add(".jpg");
                fileOpenPicker.FileTypeFilter.Add(".png");
                fileOpenPicker.ViewMode = PickerViewMode.Thumbnail;
                StorageFile selectedStorageFile = await fileOpenPicker.PickSingleFileAsync();

                if (selectedStorageFile == null)
                {
                    return null;
                }

                return await LoadVideoFrameFromStorageFileAsync(selectedStorageFile);
            });
        }

        /// <summary>
        /// Decode image from a StorageFile and return a VideoFrame
        /// </summary>
        /// <param name="file"></param>
        /// <returns></returns>
        public static IAsyncOperation<VideoFrame> LoadVideoFrameFromStorageFileAsync(StorageFile file)
        {
            return AsyncInfo.Run(async (token) =>
            {
                VideoFrame resultFrame = null;
                SoftwareBitmap softwareBitmap;
                using (IRandomAccessStream stream = await file.OpenAsync(FileAccessMode.Read))
                {
                    // Create the decoder from the stream 
                    BitmapDecoder decoder = await BitmapDecoder.CreateAsync(stream);

                    // Get the SoftwareBitmap representation of the file in BGRA8 format
                    softwareBitmap = await decoder.GetSoftwareBitmapAsync();
                    softwareBitmap = SoftwareBitmap.Convert(softwareBitmap, BitmapPixelFormat.Bgra8, BitmapAlphaMode.Premultiplied);
                }

                // Encapsulate the image in the WinML image type (VideoFrame) to be bound and evaluated
                resultFrame = VideoFrame.CreateWithSoftwareBitmap(softwareBitmap);

                return resultFrame;
            });
        }

        /// <summary>
        /// Launch file picker for user to select a file and save a VideoFrame to it
        /// </summary>
        /// <param name="frame"></param>
        /// <returns></returns>
        public static IAsyncAction SaveVideoFrameToFilePickedAsync(VideoFrame frame)
        {
            return AsyncInfo.Run(async (token) =>
            {
                // Trigger file picker to select an image file
                FileSavePicker fileSavePicker = new FileSavePicker();
                fileSavePicker.SuggestedStartLocation = PickerLocationId.PicturesLibrary;
                fileSavePicker.FileTypeChoices.Add("image file", new List<string>() { ".jpg" });
                fileSavePicker.SuggestedFileName = "NewImage";

                StorageFile selectedStorageFile = await fileSavePicker.PickSaveFileAsync();

                if (selectedStorageFile == null)
                {
                    return;
                }

                using (IRandomAccessStream stream = await selectedStorageFile.OpenAsync(FileAccessMode.ReadWrite))
                {
                    VideoFrame frameToEncode = frame;
                    BitmapEncoder encoder = await BitmapEncoder.CreateAsync(BitmapEncoder.JpegEncoderId, stream);

                    if (frameToEncode.SoftwareBitmap == null)
                    {
                        Debug.Assert(frame.Direct3DSurface != null);
                        frameToEncode = new VideoFrame(BitmapPixelFormat.Bgra8, frame.Direct3DSurface.Description.Width, frame.Direct3DSurface.Description.Height);
                        await frame.CopyToAsync(frameToEncode);
                    }
                    encoder.SetSoftwareBitmap(
                        frameToEncode.SoftwareBitmap.BitmapPixelFormat.Equals(BitmapPixelFormat.Bgra8) ?
                        frameToEncode.SoftwareBitmap
                        : SoftwareBitmap.Convert(frameToEncode.SoftwareBitmap, BitmapPixelFormat.Bgra8));

                    await encoder.FlushAsync();
                }
            });
        }
    }
}