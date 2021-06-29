using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Media.Imaging;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Navigation;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.Foundation;
using Windows.Foundation.Collections;
using AudioPreprocessing.ViewModel;
using System.Windows.Forms;
using Windows.Graphics.Imaging;
using Windows.UI.Core;
using Windows.Storage;
using System.Runtime.InteropServices;
using Windows.Storage.Pickers;
using WinRT;

// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.

namespace AudioPreprocessing
{
    /// <summary>
    /// An empty window that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class MainWindow : Window
    {
        public MainWindow()
        {
            this.InitializeComponent();
            this.ViewModel = new PreprocessViewModel();
        }

        [ComImport, System.Runtime.InteropServices.Guid("3E68D4BD-7135-4D10-8018-9FB6D9F33FA1"), InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IInitializeWithWindow
        {
            void Initialize([In] IntPtr hwnd);
        }

        [DllImport("user32.dll", ExactSpelling = true, CharSet = CharSet.Auto, PreserveSig = true, SetLastError = false)]
        public static extern IntPtr GetActiveWindow();
        public PreprocessViewModel ViewModel { get; set; }
        private async void OnOpenClick(object sender, RoutedEventArgs e)
        {

            // Open a text file.
            FileOpenPicker open = new FileOpenPicker();
            open.SuggestedStartLocation = PickerLocationId.DocumentsLibrary;
            open.FileTypeFilter.Add(".wav");

            // When running on win32, FileOpenPicker needs to know the top-level hwnd via IInitializeWithWindow::Initialize.
            if (Window.Current == null)
            {
                IInitializeWithWindow initializeWithWindowWrapper = open.As<IInitializeWithWindow>();
                IntPtr hwnd = GetActiveWindow();
                initializeWithWindowWrapper.Initialize(hwnd);
            }

            StorageFile file = await open.PickSingleFileAsync();

            SoftwareBitmap softwareBitmap;

            using (Windows.Storage.Streams.IRandomAccessStream stream = file.OpenAsync(FileAccessMode.Read).GetAwaiter().GetResult())
            {
                // Create the decoder from the stream
                BitmapDecoder decoder = BitmapDecoder.CreateAsync(stream).GetAwaiter().GetResult();

                // Get the SoftwareBitmap representation of the file
                softwareBitmap = decoder.GetSoftwareBitmapAsync().GetAwaiter().GetResult();
            }

            //var softwareBitmap = ViewModel.ProcessFile();
            //var softwareBitmap = new SoftwareBitmap(BitmapPixelFormat.Bgra8, 100, 200, BitmapAlphaMode.Premultiplied);

            if (softwareBitmap.BitmapPixelFormat != BitmapPixelFormat.Bgra8 ||
                softwareBitmap.BitmapAlphaMode != BitmapAlphaMode.Premultiplied)
            {
                softwareBitmap = SoftwareBitmap.Convert(softwareBitmap, BitmapPixelFormat.Bgra8, BitmapAlphaMode.Premultiplied);
            }

            WavFilePath.Text = ViewModel.AudioPath;
            var source = new SoftwareBitmapSource();

            //Dispatcher.RunAsync(CoreDispatcherPriority.Normal,  async () => { 
            //source.SetBitmapAsync(softwareBitmap);
            //spectrogram.Source = source;
            //});
        }

    }
}
