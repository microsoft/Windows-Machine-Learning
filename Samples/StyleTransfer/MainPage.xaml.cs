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

        private MediaPlayer _mediaPlayer;
        Windows.Media.Capture.MediaCapture _mediaCapture;
        private List<MediaFrameSourceGroup> _mediaFrameSourceGroupList;
        private MediaFrameSourceGroup _selectedMediaFrameSourceGroup;
        private MediaFrameSource _selectedMediaFrameSource;

        public MainPage()
        {
            this.InitializeComponent();
            _viewModel = new AppViewModel();
            this.DataContext = _viewModel;


        }

        private async void UIButtonLiveStream_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                // Find the sources 
                var allGroups = await MediaFrameSourceGroup.FindAllAsync();
                _mediaFrameSourceGroupList = allGroups.Where(group => group.SourceInfos.Any(sourceInfo => sourceInfo.SourceKind == MediaFrameSourceKind.Color
                                                                                                         && (sourceInfo.MediaStreamType == MediaStreamType.VideoPreview
                                                                                                         || sourceInfo.MediaStreamType == MediaStreamType.VideoRecord))).ToList();
            }
            catch (Exception ex)
            {
                Debug.WriteLine(ex.Message);
                _mediaFrameSourceGroupList = null;
            }

            if ((_mediaFrameSourceGroupList == null) || (_mediaFrameSourceGroupList.Count == 0))
            {
                // No camera sources found
                Debug.WriteLine("No Camera found");
                return;
            }

            var cameraNamesList = _mediaFrameSourceGroupList.Select(group => group.DisplayName);
            UICmbCamera.ItemsSource = cameraNamesList;
            UICmbCamera.SelectedIndex = 0;

            // UICmbCamera_SelectionChanged
            try
            {
                _selectedMediaFrameSourceGroup = _mediaFrameSourceGroupList[UICmbCamera.SelectedIndex];

                // Create MediaCapture and its settings
                _mediaCapture = new MediaCapture();
                var settings = new MediaCaptureInitializationSettings
                {
                    SourceGroup = _selectedMediaFrameSourceGroup,
                    PhotoCaptureSource = PhotoCaptureSource.Auto,
                    MemoryPreference = MediaCaptureMemoryPreference.Cpu,
                    StreamingCaptureMode = StreamingCaptureMode.Video
                };

                // Initialize MediaCapture
                await _mediaCapture.InitializeAsync(settings);
                StartPreview();
            }
            catch (Exception ex)
            {
                Debug.WriteLine(ex.ToString());
            }
        }

        private void StartPreview()
        {
            Debug.WriteLine("StartPreview");
            _selectedMediaFrameSource = _mediaCapture.FrameSources.FirstOrDefault(source => source.Value.Info.MediaStreamType == MediaStreamType.VideoPreview
                                                                                  && source.Value.Info.SourceKind == MediaFrameSourceKind.Color).Value;
            if (_selectedMediaFrameSource == null)
            {
                _selectedMediaFrameSource = _mediaCapture.FrameSources.FirstOrDefault(source => source.Value.Info.MediaStreamType == MediaStreamType.VideoRecord
                                                                                      && source.Value.Info.SourceKind == MediaFrameSourceKind.Color).Value;
            }

            // if no preview stream are available, bail
            if (_selectedMediaFrameSource == null)
            {
                return;
            }

            _mediaPlayer = new MediaPlayer();
            _mediaPlayer.RealTimePlayback = true;
            _mediaPlayer.AutoPlay = true;
            _mediaPlayer.Source = MediaSource.CreateFromMediaFrameSource(_selectedMediaFrameSource);
            UIInputMediaPlayerElement.SetMediaPlayer(_mediaPlayer);
            UIOutputMediaPlayerElement.SetMediaPlayer(_mediaPlayer);

            UITxtBlockPreviewProperties.Text = string.Format("{0}x{1}@{2}, {3}",
                        _selectedMediaFrameSource.CurrentFormat.VideoFormat.Width,
                        _selectedMediaFrameSource.CurrentFormat.VideoFormat.Height,
                        _selectedMediaFrameSource.CurrentFormat.FrameRate.Numerator + "/" + _selectedMediaFrameSource.CurrentFormat.FrameRate.Denominator,
                        _selectedMediaFrameSource.CurrentFormat.Subtype);

            UICameraSelectionControls.Visibility = Visibility.Visible;
            UIInputMediaPlayerElement.Visibility = Visibility.Visible;
            UIOutputMediaPlayerElement.Visibility = Visibility.Visible;

        }
    }
}
