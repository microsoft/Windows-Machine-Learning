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
using Windows.Graphics.Imaging;
using Windows.UI.Xaml.Media.Imaging;

namespace StyleTransfer
{
    class AppViewModel : INotifyPropertyChanged
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
        private LearningModelBinding m_binding;
        string m_inputImageDescription;
        string m_outputImageDescription;
        IMediaExtension videoEffect;

        // Image style transfer properties
        private bool _showInitialImage;
        uint m_inWidth, m_inHeight, m_outWidth, m_outHeight;
        private string _DefaultImageFileName = "Input.jpg";


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

        private SoftwareBitmapSource _inputSoftwareBitmapSource;
        public SoftwareBitmapSource InputSoftwareBitmapSource
        {
            get { return _inputSoftwareBitmapSource; }
            set { _inputSoftwareBitmapSource = value; OnPropertyChanged(); }
        }
        private SoftwareBitmapSource _outputSoftwareBitmapSource;
        public SoftwareBitmapSource OutputSoftwareBitmapSource
        {
            get { return _outputSoftwareBitmapSource; }
            set { _outputSoftwareBitmapSource = value; OnPropertyChanged(); }
        }

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
            CleanupInputImage();

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
                    await StartFilePick();
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
                    await ChangeFilePick();
                    break;
                case "Inking":
                    break;
            }
        }

        public async Task StartFilePick()
        {
            Debug.WriteLine("StartFilePick");
            InputSoftwareBitmapSource = new SoftwareBitmapSource();

            try
            {
                // Load image to VideoFrame
                _appModel.InputFrame = await ImageHelper.LoadVideoFrameFromFilePickedAsync();
                if (_appModel.InputFrame == null)
                {
                    Debug.WriteLine("no valid image file selected");
                }
                else
                {
                    await ChangeFilePick();
                }

            }
            catch (Exception ex)
            {
                Debug.WriteLine($"error: {ex.Message}");
                //NotifyUser(ex.Message, NotifyType.ErrorMessage);
            }
        }

        public async Task ChangeFilePick()
        {
            // Make sure have an input image, use default otherwise
            if (_appModel.InputFrame == null)
            {
                var file = await StorageFile.GetFileFromApplicationUriAsync(new Uri($"ms-appx:///Assets/{_DefaultImageFileName}"));
                _appModel.InputFrame = await ImageHelper.LoadVideoFrameFromStorageFileAsync(file);
            }

            await EvaluateVideoFrameAsync();
        }

        private async Task EvaluateVideoFrameAsync()
        {
            OutputSoftwareBitmapSource = new SoftwareBitmapSource();

            if ((_appModel.InputFrame != null) &&
                (_appModel.InputFrame.SoftwareBitmap != null || _appModel.InputFrame.Direct3DSurface != null))
            {
                _appModel.InputFrame = await ImageHelper.CenterCropImageAsync(_appModel.InputFrame, m_inWidth, m_inHeight);

                m_binding.Bind(m_inputImageDescription, ImageFeatureValue.CreateFromVideoFrame(_appModel.InputFrame));
                m_binding.Bind(m_outputImageDescription, ImageFeatureValue.CreateFromVideoFrame(_appModel.OutputFrame));

                var results = m_session.Evaluate(m_binding, "test");

                // Parse Results
                IReadOnlyDictionary<string, object> outputs = results.Outputs;
                foreach (var output in outputs)
                {
                    Debug.WriteLine($"{output.Key} : {output.Value} -> {output.Value.GetType()}");
                }

                await InputSoftwareBitmapSource.SetBitmapAsync(_appModel.InputFrame.SoftwareBitmap);
                await OutputSoftwareBitmapSource.SetBitmapAsync(_appModel.OutputFrame.SoftwareBitmap);

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
                    { "Session", m_session},
                    { "Binding", m_binding },
                    { "InputImageDescription", m_inputImageDescription },
                    { "OutputImageDescription", m_outputImageDescription } });

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
            m_binding = new LearningModelBinding(m_session);

            debugModelIO();

            m_inputImageDescription = m_model.InputFeatures.ToList().First().Name;

            m_outputImageDescription = m_model.OutputFeatures.ToList().First().Name;
        }

        public void debugModelIO()
        {
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

        private void CleanupInputImage()
        {
            InputSoftwareBitmapSource?.Dispose();
            OutputSoftwareBitmapSource?.Dispose();

            _appModel.OutputFrame?.Dispose();
            _appModel.OutputFrame = new VideoFrame(BitmapPixelFormat.Bgra8, (int)m_outWidth, (int)m_outHeight);
        }

        public event PropertyChangedEventHandler PropertyChanged;
        private void OnPropertyChanged([CallerMemberName] string propertyName = null)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }
    }
}
