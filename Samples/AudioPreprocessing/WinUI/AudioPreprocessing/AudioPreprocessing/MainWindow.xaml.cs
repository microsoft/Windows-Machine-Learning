using AudioPreprocessing.Model;
using AudioPreprocessing.ViewModel;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Media.Imaging;
using System;
using System.Runtime.InteropServices;
using System.Threading.Tasks;
using Windows.Graphics.Imaging;
using Windows.Media;
using Windows.Storage;
using Windows.Storage.Pickers;

// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.

namespace AudioPreprocessing
{
    [ComImport, Guid("3E68D4BD-7135-4D10-8018-9FB6D9F33FA1"), InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
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
            InitializeComponent();
            ViewModel = new PreprocessViewModel();
            spectrogram.Source = new SoftwareBitmapSource();
        }

        public PreprocessViewModel ViewModel { get; set; }
        private async void OnOpenClick(object sender, RoutedEventArgs e)
        {
            string wavPath = await GetFilePath();

            ViewModel.GenerateMelSpectrograms(wavPath, ColorMelSpectrogramCheckBox.IsChecked ?? false);
            WavFilePath.Text = ViewModel.AudioPath;

            await ((SoftwareBitmapSource)spectrogram.Source).SetBitmapAsync(ViewModel.MelSpectrogramImage);
        }

        private async Task<string> GetFilePath()
        { 
            FileOpenPicker openPicker = new FileOpenPicker();
            openPicker.ViewMode = PickerViewMode.Thumbnail;
            openPicker.FileTypeFilter.Add(".wav");

            // When running on win32, FileOpenPicker needs to know the top-level hwnd via IInitializeWithWindow::Initialize.
            if (Window.Current == null)
            {
                var picker_unknown = Marshal.GetComInterfaceForObject(openPicker, typeof(IInitializeWithWindow));
                var initializeWithWindowWrapper = (IInitializeWithWindow)Marshal.GetTypedObjectForIUnknown(picker_unknown, typeof(IInitializeWithWindow));
                IntPtr hwnd = GetActiveWindow();
                initializeWithWindowWrapper.Initialize(hwnd);
            }

            StorageFile file = await openPicker.PickSingleFileAsync();
            return file.Path;
        }
    }
}
