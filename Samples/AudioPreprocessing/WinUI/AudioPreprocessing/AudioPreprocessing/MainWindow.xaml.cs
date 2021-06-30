using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Media.Imaging;
using System;
using AudioPreprocessing.ViewModel;
using Windows.Graphics.Imaging;
using Windows.Storage;
using System.Runtime.InteropServices;
using Windows.Storage.Pickers;
using AudioPreprocessing.Model;

// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.

namespace AudioPreprocessing
{
    [ComImport, System.Runtime.InteropServices.Guid("3E68D4BD-7135-4D10-8018-9FB6D9F33FA1"), InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    public interface IInitializeWithWindow
    {
        void Initialize([In] IntPtr hwnd);
    }

    /// <summary>
    /// Main Window of the app, displays melspectrograms.
    /// </summary>
    public sealed partial class MainWindow : Window
    {
        [DllImport("user32.dll", ExactSpelling = true, CharSet = CharSet.Auto, PreserveSig = true, SetLastError = false)]
        public static extern IntPtr GetActiveWindow();
        public MainWindow()
        {
            this.InitializeComponent();
            this.ViewModel = new PreprocessViewModel();
        }

        public PreprocessViewModel ViewModel { get; set; }
        private async void OnOpenClick(object sender, RoutedEventArgs e)
        {

            // Open a text file.
            FileOpenPicker openPicker = new FileOpenPicker();
            openPicker.ViewMode = PickerViewMode.Thumbnail;
            openPicker.FileTypeFilter.Add(".wav");

            // When running on win32, FileOpenPicker needs to know the top-level hwnd via IInitializeWithWindow::Initialize.
            if (Window.Current == null)
            {
                var picker_unknown = Marshal.GetComInterfaceForObject(openPicker, typeof(IInitializeWithWindow));
                IInitializeWithWindow initializeWithWindowWrapper = Marshal.GetTypedObjectForIUnknown(picker_unknown, typeof(IInitializeWithWindow)) as IInitializeWithWindow;
                IntPtr hwnd = GetActiveWindow();
                initializeWithWindowWrapper.Initialize(hwnd);
            }

            StorageFile file = await openPicker.PickSingleFileAsync();
            PreprocessModel melspec = new PreprocessModel();
            var softwareBitmap = melspec.GenerateMelSpectrogram(file.Path);

            ViewModel.AudioPath = file.Path;
            ViewModel.MelSpectrogramImage = softwareBitmap;

            if (softwareBitmap.BitmapPixelFormat != BitmapPixelFormat.Bgra8 ||
                softwareBitmap.BitmapAlphaMode != BitmapAlphaMode.Premultiplied)
            {
                softwareBitmap = SoftwareBitmap.Convert(softwareBitmap, BitmapPixelFormat.Bgra8, BitmapAlphaMode.Premultiplied);
            }


            WavFilePath.Text = file.Path;
            var source = new SoftwareBitmapSource();
            await source.SetBitmapAsync(softwareBitmap);
            spectrogram.Source = source;
        }
    }
}
