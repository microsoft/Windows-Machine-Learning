using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.AI.MachineLearning;
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
        /// Crop image given a imageVariableDescription 
        /// </summary>
        /// <param name="inputVideoFrame"></param>
        /// <returns></returns>
        public static IAsyncOperation<VideoFrame> CenterCropImageAsync(VideoFrame inputVideoFrame, ImageFeatureDescriptor imageVariableDescription)
        {
            return CenterCropImageAsync(inputVideoFrame, imageVariableDescription.Width, imageVariableDescription.Height);
        }

        /// <summary>
        /// Crop image given a target width and height 
        /// </summary>
        /// <param name="inputVideoFrame"></param>
        /// <returns></returns>
        public static IAsyncOperation<VideoFrame> CenterCropImageAsync(VideoFrame inputVideoFrame, uint targetWidth, uint targetHeight)
        {
            return AsyncInfo.Run(async (token) =>
            {
                bool useDX = inputVideoFrame.SoftwareBitmap == null;
                VideoFrame result = null;
                // Center crop
                try
                {

                    // Since we will be center-cropping the image, figure which dimension has to be clipped
                    var frameHeight = useDX ? inputVideoFrame.Direct3DSurface.Description.Height : inputVideoFrame.SoftwareBitmap.PixelHeight;
                    var frameWidth = useDX ? inputVideoFrame.Direct3DSurface.Description.Width : inputVideoFrame.SoftwareBitmap.PixelWidth;

                    Rect cropRect = GetCropRect(frameWidth, frameHeight, targetWidth, targetHeight);
                    BitmapBounds cropBounds = new BitmapBounds()
                    {
                        Width = (uint)cropRect.Width,
                        Height = (uint)cropRect.Height,
                        X = (uint)cropRect.X,
                        Y = (uint)cropRect.Y
                    };

                    // Create the VideoFrame to be bound as input for evaluation
                    if (useDX)
                    {
                        if (inputVideoFrame.Direct3DSurface == null)
                        {
                            throw (new Exception("Invalid VideoFrame without SoftwareBitmap nor D3DSurface"));
                        }

                        result = new VideoFrame(BitmapPixelFormat.Bgra8,
                                                (int)targetWidth,
                                                (int)targetHeight,
                                                BitmapAlphaMode.Premultiplied);
                    }
                    else
                    {
                        result = new VideoFrame(BitmapPixelFormat.Bgra8,
                                                (int)targetWidth,
                                                (int)targetHeight,
                                                BitmapAlphaMode.Premultiplied);
                    }

                    await inputVideoFrame.CopyToAsync(result, cropBounds, null);
                }
                catch (Exception ex)
                {
                    Debug.WriteLine(ex.ToString());
                }

                return result;
            });
        }

        /// <summary>
        /// Calculate the center crop bounds given a set of source and target dimensions
        /// </summary>
        /// <param name="frameWidth"></param>
        /// <param name="frameHeight"></param>
        /// <param name="targetWidth"></param>
        /// <param name="targetHeight"></param>
        /// <returns></returns>
        public static Rect GetCropRect(int frameWidth, int frameHeight, uint targetWidth, uint targetHeight)
        {
            Rect rect = new Rect();

            // we need to recalculate the crop bounds in order to correctly center-crop the input image
            float flRequiredAspectRatio = (float)targetWidth / targetHeight;

            if (flRequiredAspectRatio * frameHeight > (float)frameWidth)
            {
                // clip on the y axis
                rect.Height = (uint)Math.Min((frameWidth / flRequiredAspectRatio + 0.5f), frameHeight);
                rect.Width = (uint)frameWidth;
                rect.X = 0;
                rect.Y = (uint)(frameHeight - rect.Height) / 2;
            }
            else // clip on the x axis
            {
                rect.Width = (uint)Math.Min((flRequiredAspectRatio * frameHeight + 0.5f), frameWidth);
                rect.Height = (uint)frameHeight;
                rect.X = (uint)(frameWidth - rect.Width) / 2; ;
                rect.Y = 0;
            }
            return rect;
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
        /// Launch file picker for user to select a picture file and return a VideoFrame
        /// </summary>
        /// <returns>VideoFrame instanciated from the selected image file</returns>
        public static IAsyncOperation<StorageFile> GetSelectedStorageFile()
        {
            return AsyncInfo.Run(async (token) =>
            {
                // Trigger file picker to select an image file
                FileOpenPicker fileOpenPicker = new FileOpenPicker();
                fileOpenPicker.SuggestedStartLocation = PickerLocationId.PicturesLibrary;
                fileOpenPicker.FileTypeFilter.Add(".mp4");
                fileOpenPicker.FileTypeFilter.Add(".mkv");
                fileOpenPicker.ViewMode = PickerViewMode.Thumbnail;
                StorageFile selectedStorageFile = await fileOpenPicker.PickSingleFileAsync();

                if (selectedStorageFile == null)
                {
                    return null;
                }

                return selectedStorageFile;
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
