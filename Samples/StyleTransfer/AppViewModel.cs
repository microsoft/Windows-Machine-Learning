using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Input;
using Windows.Media.Core;
using GalaSoft.MvvmLight.Command;
using Windows.Media.Capture.Frames;
using System.Diagnostics;
using Windows.Media.Capture;
using Windows.Media.Playback;
using Windows.Media.Effects;
using Windows.Media;
using Windows.Foundation.Collections;

namespace StyleTransfer
{
    class AppViewModel : INotifyPropertyChanged
    {
        public AppViewModel()
        {
            _appModel = new AppModel();
            // TODO: Add a param to check if have an image to save. 
            SaveCommand = new RelayCommand(this.SaveOutput);
            MediaSourceCommand = new RelayCommand<string>(SetMediaSource);
            LiveStreamCommand = new RelayCommand(async () => await StartWebcamStream());
        }

        Windows.Media.Capture.MediaCapture _mediaCapture;
        private List<MediaFrameSourceGroup> _mediaFrameSourceGroupList;
        private MediaFrameSourceGroup _selectedMediaFrameSourceGroup;
        private MediaFrameSource _selectedMediaFrameSource;

        private AppModel _appModel;
        public AppModel CurrentApp
        {
            get { return _appModel; }
        }

        private ICommand _saveCommand;
        public ICommand SaveCommand
        {
            get { return _saveCommand; }
            set
            {
                _saveCommand = value;
            }
        }

        public ICommand MediaSourceCommand { get; set; }
        public ICommand LiveStreamCommand { get; set; }

        public event PropertyChangedEventHandler PropertyChanged;

        public void SaveOutput()
        {
            // TODO: Take from UIButtonSaveImage_Click
            return;
        }

        public async Task StartWebcamStream()
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

            _appModel.CameraNamesList = _mediaFrameSourceGroupList.Select(group => group.DisplayName);

            // UICmbCamera_SelectionChanged
            try
            {
                _selectedMediaFrameSourceGroup = _mediaFrameSourceGroupList[_appModel.SelectedCameraIndex];

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

                // Initialize VideoEffect
                var videoEffectDefinition = new VideoEffectDefinition("VideoEffectComponent.StyleTransferVideoEffect");
                IMediaExtension videoEffect = await _mediaCapture.AddVideoEffectAsync(videoEffectDefinition, MediaStreamType.VideoPreview);
                videoEffect.SetProperties(new PropertySet() { { "FadeValue", .15 } });

                StartPreview();
            }
            catch (Exception ex)
            {
                Debug.WriteLine(ex.ToString());
            }
        }


        public void SetMediaSource(object obj)
        {
            // TODO: Convert to a better value for the appModel object here. 
            _appModel.InputMedia = obj.ToString();

            // If video source, whole different code flow

            // If camera/image upload, gather input image then put to eval/render
            // If source == upload --> HelperMethods::LoadVideoFrameFromFilePickedAsync

            // Process everythign and then set MediaPlkayer object in AppModel for input/output
            return;
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

            var _mediaPlayer = new MediaPlayer();
            _mediaPlayer.RealTimePlayback = true;
            _mediaPlayer.AutoPlay = true;
            _appModel.OutputMediaSource = MediaSource.CreateFromMediaFrameSource(_selectedMediaFrameSource);
            _appModel.InputMediaSource = MediaSource.CreateFromMediaFrameSource(_selectedMediaFrameSource);


            //UIInputMediaPlayerElement.SetMediaPlayer(_mediaPlayer);
            //UIOutputMediaPlayerElement.SetMediaPlayer(_mediaPlayer);

            /*UITxtBlockPreviewProperties.Text = string.Format("{0}x{1}@{2}, {3}",
                        _selectedMediaFrameSource.CurrentFormat.VideoFormat.Width,
                        _selectedMediaFrameSource.CurrentFormat.VideoFormat.Height,
                        _selectedMediaFrameSource.CurrentFormat.FrameRate.Numerator + "/" + _selectedMediaFrameSource.CurrentFormat.FrameRate.Denominator,
                        _selectedMediaFrameSource.CurrentFormat.Subtype);

            UICameraSelectionControls.Visibility = Visibility.Visible;
            UIInputMediaPlayerElement.Visibility = Visibility.Visible;
            UIOutputMediaPlayerElement.Visibility = Visibility.Visible;*/

        }
    }
}
