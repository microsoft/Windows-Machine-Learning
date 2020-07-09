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
using Windows.AI.MachineLearning;
using Windows.Media.Playback;
using Windows.Media.Core;
using Windows.Media.Capture;
using Windows.Media.MediaProperties;
using Windows.Storage;
using Windows.Media.Capture.Frames;
using System.Diagnostics;
using Windows.System.Display;
using Windows.Graphics.Display;
using System.Threading.Tasks;
using Windows.Media.Effects;

// The Blank Page item template is documented at https://go.microsoft.com/fwlink/?LinkId=402352&clcid=0x409

namespace StyleTransfer
{


    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class MainPage : Page
    {

        private AppViewModel _viewModel;
        public readonly List<string> modelFileNames = new List<string>
        {   "candy",
            "mosaic",
            "la_muse",
            "udnie"
        };
        public int dummyFPS = 0;
        DisplayRequest displayRequest = new DisplayRequest();

        public MainPage()
        {
            this.InitializeComponent();
            _viewModel = new AppViewModel();
            this.DataContext = _viewModel;
        }

        public async void UIButtonLiveStream_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                _viewModel._mediaCapture = new MediaCapture();
                await _viewModel._mediaCapture.InitializeAsync();
            }
            catch (UnauthorizedAccessException)
            {
                // This will be thrown if the user denied access to the camera in privacy settings
                Debug.WriteLine("The app was denied access to the camera");
                return;
            }
            try
            {
                //PreviewControl.Source = _viewModel._mediaCapture;

                _viewModel.videoEffectDefinition = new VideoEffectDefinition(_viewModel._videoEffectID);
                _viewModel.videoEffect = await _viewModel._mediaCapture.AddVideoEffectAsync(_viewModel.videoEffectDefinition, MediaStreamType.VideoPreview);
                _viewModel.videoEffect.SetProperties(new PropertySet() {
                    { "Session", _viewModel.m_session},
                    { "Binding", _viewModel.m_binding },
                    { "InputImageDescription", _viewModel.m_inputImageDescription },
                    { "OutputImageDescription", _viewModel.m_outputImageDescription } });

                await _viewModel._mediaCapture.StartPreviewAsync();
                _viewModel.isPreviewing = true;
            }
            catch (System.IO.FileLoadException)
            {
                Debug.WriteLine("File Load Exception");
                return;
            }
        }

    }

}
