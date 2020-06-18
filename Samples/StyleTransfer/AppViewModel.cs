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
using Windows.AI.MachineLearning;
using Windows.Storage;
using System.Runtime.CompilerServices;

namespace StyleTransfer
{
    class AppViewModel
    {
        public AppViewModel()
        {
            _appModel = new AppModel();
            SaveCommand = new RelayCommand(SaveOutput);
            SetMediaSourceCommand = new RelayCommand<string>(async (x) => await SetMediaSource(x));
            SetModelSourceCommand = new RelayCommand(async () => await SetModelSource());
            ChangeLiveStreamCommand = new RelayCommand(async () => await ChangeLiveStream());
        }

        // Media capture properties
        Windows.Media.Capture.MediaCapture _mediaCapture;
        private List<MediaFrameSourceGroup> _mediaFrameSourceGroupList;
        private MediaFrameSourceGroup _selectedMediaFrameSourceGroup;
        private MediaFrameSource _selectedMediaFrameSource;

        // Style transfer effect properties
        private LearningModel m_model = null;
        private LearningModelDeviceKind m_inferenceDeviceSelected = LearningModelDeviceKind.Default;
        private LearningModelSession m_session;
        string _inputImageDescription;
        string _outputImageDescription;
        IMediaExtension videoEffect;


        private AppModel _appModel;
        public AppModel CurrentApp
        {
            get { return _appModel; }
        }

        // Command defintions
        public ICommand SaveCommand { get; set; }
        public ICommand SetMediaSourceCommand { get; set; }
        public ICommand ChangeLiveStreamCommand { get; set; }
        public ICommand SetModelSourceCommand { get; set; }

        public void SaveOutput()
        {
            // TODO: Take from UIButtonSaveImage_Click
            return;
        }

        public async Task SetMediaSource(string obj)
        {
            _appModel.InputMedia = obj;

            // TODO: Reset media source stuff: set Camera input controls visibility to 0, etc. 
            await CleanupCameraAsync();
            // TODO: CleanupInputImage()

            // Changes media source while keeping all other controls 
            switch (_appModel.InputMedia)
            {
                case "LiveStream":
                    await StartLiveStream();
                    // TODO: Also spin up a Capture for preview on left side
                    break;
                case "AcquireImage":
                    break;
                case "FilePick":
                    // HelperMethods::LoadVideoFrameFromFilePickedAsync
                    break;
                case "Inking":
                    break;
            }

            return;
        }

        public async Task SetModelSource()
        {
            // Clean up model, etc. by setting to null? 
            // Based on InputMedia, call ChangeLiveStream/ChangeCamera/ChangeImage
            await LoadModelAsync();
            switch (_appModel.InputMedia)
            {
                case "LiveStream":
                    await ChangeLiveStream();
                    // TODO: Also spin up a Capture for preview on left side
                    break;
                case "AcquireImage":
                    break;
                case "FilePick":
                    break;
                case "Inking":
                    break;
            }
        }

        public async Task StartLiveStream()
        {
            Debug.WriteLine("StartLiveStream");

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
            _appModel.SelectedCameraIndex = 0;
        }

        public async Task ChangeLiveStream()
        {
            Debug.WriteLine("ChangeLiveStream");

            // If webcam hasn't been initialized, bail.
            if (_mediaFrameSourceGroupList == null) { return; }

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
                VideoEffectDefinition videoEffectDefinition = new VideoEffectDefinition("StyleTransferEffectComponent.StyleTransferVideoEffect");
                videoEffect = await _mediaCapture.AddVideoEffectAsync(videoEffectDefinition, MediaStreamType.VideoPreview);
                videoEffect.SetProperties(new PropertySet() {
                    { "Model", m_model},
                    { "Session", m_session },
                    { "InputImageDescription", _inputImageDescription },
                    { "OutputImageDescription", _outputImageDescription } });

                StartPreview();
            }
            catch (Exception ex)
            {
                Debug.WriteLine(ex.ToString());
            }
        }

        // Preview the input and output media 
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

            // If no preview stream are available, bail
            if (_selectedMediaFrameSource == null)
            {
                return;
            }

            _appModel.OutputMediaSource = MediaSource.CreateFromMediaFrameSource(_selectedMediaFrameSource);
            _appModel.InputMediaSource = MediaSource.CreateFromMediaFrameSource(_selectedMediaFrameSource);

        }

        private async Task LoadModelAsync()
        {

            StorageFile modelFile = await StorageFile.GetFileFromApplicationUriAsync(new Uri($"ms-appx:///Assets/{_appModel.ModelSource}.onnx"));
            m_model = await LearningModel.LoadFromStorageFileAsync(modelFile);

            // TODO: Pass in useGPU as well.
            m_inferenceDeviceSelected = LearningModelDeviceKind.Cpu;
            m_session = new LearningModelSession(m_model, new LearningModelDevice(m_inferenceDeviceSelected));

            debugModelIO();

            _inputImageDescription = m_model.InputFeatures.ToList().First().Name;

            _outputImageDescription = m_model.OutputFeatures.ToList().First().Name;
        }

        public void debugModelIO()
        {
            uint m_inWidth, m_inHeight, m_outWidth, m_outHeight;
            string m_inName, m_outName;
            foreach (var inputF in m_model.InputFeatures)
            {
                Debug.WriteLine($"input | kind:{inputF.Kind}, name:{inputF.Name}, type:{inputF.GetType()}");
                int i = 0;
                ImageFeatureDescriptor imgDesc = inputF as ImageFeatureDescriptor;
                TensorFeatureDescriptor tfDesc = inputF as TensorFeatureDescriptor;
                m_inWidth = (uint)(imgDesc == null ? tfDesc.Shape[3] : imgDesc.Width);
                m_inHeight = (uint)(imgDesc == null ? tfDesc.Shape[2] : imgDesc.Height);
                m_inName = inputF.Name;

                Debug.WriteLine($"N: {(imgDesc == null ? tfDesc.Shape[0] : 1)}, " +
                    $"Channel: {(imgDesc == null ? tfDesc.Shape[1].ToString() : imgDesc.BitmapPixelFormat.ToString())}, " +
                    $"Height:{(imgDesc == null ? tfDesc.Shape[2] : imgDesc.Height)}, " +
                    $"Width: {(imgDesc == null ? tfDesc.Shape[3] : imgDesc.Width)}");
            }
            foreach (var outputF in m_model.OutputFeatures)
            {
                Debug.WriteLine($"output | kind:{outputF.Kind}, name:{outputF.Name}, type:{outputF.GetType()}");
                int i = 0;
                ImageFeatureDescriptor imgDesc = outputF as ImageFeatureDescriptor;
                TensorFeatureDescriptor tfDesc = outputF as TensorFeatureDescriptor;
                m_outWidth = (uint)(imgDesc == null ? tfDesc.Shape[3] : imgDesc.Width);
                m_outHeight = (uint)(imgDesc == null ? tfDesc.Shape[2] : imgDesc.Height);
                m_outName = outputF.Name;

                Debug.WriteLine($"N: {(imgDesc == null ? tfDesc.Shape[0] : 1)}, " +
                   $"Channel: {(imgDesc == null ? tfDesc.Shape[1].ToString() : imgDesc.BitmapPixelFormat.ToString())}, " +
                   $"Height:{(imgDesc == null ? tfDesc.Shape[2] : imgDesc.Height)}, " +
                   $"Width: {(imgDesc == null ? tfDesc.Shape[3] : imgDesc.Width)}");
            }
        }

        private async Task CleanupCameraAsync()
        {
            Debug.WriteLine("CleanupCameraAsync");
            try
            {
                if (_mediaCapture != null)
                {
                    if (videoEffect != null) await _mediaCapture.RemoveEffectAsync(videoEffect);
                    await _mediaCapture.StopRecordAsync();
                    _mediaCapture = null;
                }
                if (_appModel.OutputMediaSource != null)
                {
                    _appModel.OutputMediaSource = null;
                }
                if (videoEffect != null)
                {
                    videoEffect = null;
                }
                // TODO: Add inputmediasource once can show both at the same time
            }

            catch (Exception ex)
            {
                Debug.WriteLine($"CleanupCameraAsync: {ex.Message}");
            }
        }
    }
}
