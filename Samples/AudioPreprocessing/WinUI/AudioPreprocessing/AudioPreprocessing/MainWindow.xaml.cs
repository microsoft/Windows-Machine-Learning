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

        public PreprocessViewModel ViewModel { get; set; }
        private void OnOpenClick(object sender, RoutedEventArgs e)
        {

            var openFileDialog = new OpenFileDialog();
            openFileDialog.Title = "Choose an Audio File";
            openFileDialog.Filter = "sound files (*.wav)|*.wav|All files (*.*)|*.*";

            if (openFileDialog.ShowDialog() != DialogResult.OK)
            {
                return;
            }

            var file = StorageFile.GetFileFromPathAsync(openFileDialog.FileName).GetAwaiter().GetResult();
            SoftwareBitmap softwareBitmap;

            using (Windows.Storage.Streams.IRandomAccessStream stream =  file.OpenAsync(FileAccessMode.Read).GetAwaiter().GetResult())
            {
                // Create the decoder from the stream
                BitmapDecoder decoder =  BitmapDecoder.CreateAsync(stream).GetAwaiter().GetResult();

                // Get the SoftwareBitmap representation of the file
                softwareBitmap =  decoder.GetSoftwareBitmapAsync().GetAwaiter().GetResult();
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
