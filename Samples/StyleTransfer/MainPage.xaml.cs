using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Data;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Navigation;
using Microsoft.AI.MachineLearning;
using Windows.Media.Playback;
using Windows.Media.Core;
using Windows.Media.Capture;
using Windows.Media.MediaProperties;
using Windows.Storage;
using Windows.Media.Capture.Frames;
using System.Diagnostics;
using Windows.System.Display;
using Windows.Graphics.Display;

// The Blank Page item template is documented at https://go.microsoft.com/fwlink/?LinkId=402352&clcid=0x409

namespace StyleTransfer
{
    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class MainPage : Page
    {
        public int dummyFPS = 10;
        public List<string> modelFileNames = new List<string>
            {"candy",
                "mosaic",
                "la_muse",
                "udnie"
            };
        private AppViewModel _viewModel;

        public MainPage()
        {
            this.InitializeComponent();
            _viewModel = new AppViewModel();
            this.DataContext = _viewModel;


        }
        Windows.Media.Capture.MediaCapture _inputMediaCapture;
        bool isPreviewing;
        DisplayRequest displayRequest = new DisplayRequest();
        private async void UIButtonLiveStream_Click(object sender, RoutedEventArgs e)
        {
            var btn = sender as Button;
            btn.Command.Execute(btn.CommandParameter);

            // Need to have separate thread ? for style transfer section
            try
            {

                _inputMediaCapture = new MediaCapture();
                await _inputMediaCapture.InitializeAsync();

                displayRequest.RequestActive();
                DisplayInformation.AutoRotationPreferences = DisplayOrientations.Landscape;
            }
            catch (UnauthorizedAccessException)
            {
                // This will be thrown if the user denied access to the camera in privacy settings
                Debug.WriteLine("The app was denied access to the camera");
                return;
            }

            try
            {
                PreviewControl.Source = _inputMediaCapture;
                await _inputMediaCapture.StartPreviewAsync();
                isPreviewing = true;
            }
            catch (System.IO.FileLoadException fle)
            {
                Debug.WriteLine("Failed to load input media capture", fle);
                //_inputMediaCapture.CaptureDeviceExclusiveControlStatusChanged += _mediaCapture_CaptureDeviceExclusiveControlStatusChanged;
            }

            return;
        }


    }
}
