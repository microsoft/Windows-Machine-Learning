using System;
using System.Collections.Generic;
using System.Text;
using System.Runtime.InteropServices;
using Windows.Storage.Pickers;
using Windows.Storage;
using Microsoft.UI.Xaml;
using Windows.Graphics.Imaging;
using Windows.Media;
using Microsoft.UI.Xaml.Media.Imaging;

namespace WinMLSamplesGallery.Common
{
    [ComImport, Guid("3E68D4BD-7135-4D10-8018-9FB6D9F33FA1"), InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    public interface IInitializeWithWindow
    {
        void Initialize([In] IntPtr hwnd);
    }

    public static class File
    {
        [DllImport("user32.dll", ExactSpelling = true, CharSet = CharSet.Auto, PreserveSig = true, SetLastError = false)]
        public static extern IntPtr GetActiveWindow();


        public static StorageFile PickImageFiles()
        {
            FileOpenPicker openPicker = new FileOpenPicker();
            openPicker.ViewMode = PickerViewMode.Thumbnail;
            openPicker.FileTypeFilter.Add(".jpg");
            openPicker.FileTypeFilter.Add(".bmp");
            openPicker.FileTypeFilter.Add(".png");
            openPicker.FileTypeFilter.Add(".jpeg");

            // When running on win32, FileOpenPicker needs to know the top-level hwnd via IInitializeWithWindow::Initialize.
            if (Window.Current == null)
            {
                var picker_unknown = Marshal.GetComInterfaceForObject(openPicker, typeof(IInitializeWithWindow));
                var initializeWithWindowWrapper = (IInitializeWithWindow)Marshal.GetTypedObjectForIUnknown(picker_unknown, typeof(IInitializeWithWindow));
                IntPtr hwnd = GetActiveWindow();
                initializeWithWindowWrapper.Initialize(hwnd);
            }

            return openPicker.PickSingleFileAsync().GetAwaiter().GetResult();
        }
        public static BitmapDecoder CreateBitmapDecoderFromStorageFile(StorageFile file)
        {
            if (file == null)
            {
                return null;
            }

            var stream = file.OpenAsync(FileAccessMode.Read).GetAwaiter().GetResult();
            var decoder = BitmapDecoder.CreateAsync(stream).GetAwaiter().GetResult();
            return decoder;
        }

        public static SoftwareBitmap CreateSoftwareBitmapFromStorageFile(StorageFile file)
        {
            if (file == null)
            {
                return null;
            }

            var decoder = CreateBitmapDecoderFromStorageFile(file);
            return decoder.GetSoftwareBitmapAsync().GetAwaiter().GetResult();
        }

        public static BitmapDecoder PickImageFileAsBitmapDecoder()
        {
            var file = PickImageFiles();
            var bitmap = CreateBitmapDecoderFromStorageFile(file);
            return bitmap;
        }

        public static BitmapDecoder CreateBitmapDecoderFromPath(string image)
        {
            var file = StorageFile.GetFileFromApplicationUriAsync(new Uri(image)).GetAwaiter().GetResult();
            return File.CreateBitmapDecoderFromStorageFile(file);
        }

        public static SoftwareBitmap PickImageFileAsSoftwareBitmap() {
            var file = PickImageFiles();
            var bitmap = CreateSoftwareBitmapFromStorageFile(file);
            return bitmap;
        }

        public static SoftwareBitmap CreateSoftwareBitmapFromPath(string image)
        {
            var file = StorageFile.GetFileFromApplicationUriAsync(new Uri(image)).GetAwaiter().GetResult();
            return File.CreateSoftwareBitmapFromStorageFile(file);
        }
    }

    public static class RenderingHelpers
    {
        public static void BindSoftwareBitmapToImageControl(Microsoft.UI.Xaml.Controls.Image imageControl, SoftwareBitmap softwareBitmap)
        {
            SoftwareBitmap displayBitmap = softwareBitmap;
            //Image control only accepts BGRA8 encoding and Premultiplied/no alpha channel. This checks and converts
            //the SoftwareBitmap we want to bind.
            if (displayBitmap.BitmapPixelFormat != BitmapPixelFormat.Bgra8 ||
                displayBitmap.BitmapAlphaMode != BitmapAlphaMode.Premultiplied)
            {
                displayBitmap = SoftwareBitmap.Convert(displayBitmap, BitmapPixelFormat.Bgra8, BitmapAlphaMode.Premultiplied);
            }

            // get software bitmap souce
            var source = new SoftwareBitmapSource();
            source.SetBitmapAsync(displayBitmap).GetAwaiter();
            // draw the input image
            imageControl.Source = source;
        }
    }
}
