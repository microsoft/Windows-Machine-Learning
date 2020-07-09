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
using Microsoft.AI.MachineLearning;
using Windows.Storage;
using System.Runtime.CompilerServices;
using Windows.Graphics.Imaging;
using Windows.UI.Xaml.Media.Imaging;
using Windows.UI.Xaml.Media;
using Windows.Media.MediaProperties;
using Windows.Storage.Pickers;
using Windows.UI.Xaml.Controls;
using System.IO;

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

            _inputSoftwareBitmapSource = new SoftwareBitmapSource();
            _outputSoftwareBitmapSource = new SoftwareBitmapSource();
            _saveEnabled = true;
            NotifyUser(true);

        }

        // Media capture properties
        public Windows.Media.Capture.MediaCapture _mediaCapture;
        private List<MediaFrameSourceGroup> _mediaFrameSourceGroupList;
        private MediaFrameSourceGroup _selectedMediaFrameSourceGroup;
        private MediaFrameSource _selectedMediaFrameSource;
        public bool isPreviewing;


        // Style transfer effect properties
        public LearningModel m_model = null;
        public LearningModelDeviceKind m_inferenceDeviceSelected = LearningModelDeviceKind.Default;
        public LearningModelSession m_session;
        public LearningModelBinding m_binding;
        public string m_inputImageDescription;
        public string m_outputImageDescription;
        public IMediaExtension videoEffect;
        public VideoEffectDefinition videoEffectDefinition;
        // Activatable Class ID of the video effect. 
        public String _videoEffectID = "StyleTransferEffectCpp.StyleTransferEffect";

        // Image style transfer properties
        uint m_inWidth, m_inHeight, m_outWidth, m_outHeight;
        private string _DefaultImageFileName = "Input.jpg";

        // User notifiaction properties
        private bool _succeed;
        private SolidColorBrush _successColor = new SolidColorBrush(Windows.UI.Colors.Green);
        private SolidColorBrush _failColor = new SolidColorBrush(Windows.UI.Colors.Red);

        private AppModel _appModel;
        public AppModel CurrentApp
        {
            get { return _appModel; }
        }

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
        private bool _saveEnabled;
        public bool SaveEnabled
        {
            get { return _saveEnabled; }
            set { _saveEnabled = value; OnPropertyChanged(); }
        }

        public SolidColorBrush StatusBarColor
        {
            get { return _succeed ? _successColor : _failColor; }
        }

        private string _notifyMessage;
        public string StatusMessage
        {
            get { return _notifyMessage; }
            set { _notifyMessage = value; OnPropertyChanged(); OnPropertyChanged("StatusBarColor"); }
        }

        // Command defintions
        public ICommand SaveCommand { get; set; }
        public ICommand SetMediaSourceCommand { get; set; }
        public ICommand ChangeLiveStreamCommand { get; set; }
        public ICommand SetModelSourceCommand { get; set; }

        public async void SaveOutput()
        {
            Debug.WriteLine("UIButtonSaveImage_Click");
            if (_appModel.InputMedia != "LiveStream") await ImageHelper.SaveVideoFrameToFilePickedAsync(_appModel.OutputFrame);
        }

        public async Task SetMediaSource(string src)
        {
            _appModel.InputMedia = src;

            CleanupCameraAsync();

            CleanupInputImage();
            NotifyUser(true);
            SaveEnabled = true;

            // Changes media source while keeping all other controls 
            switch (_appModel.InputMedia)
            {
                case "LiveStream":
                    await StartLiveStream();
                    break;
                case "AcquireImage":
                    await StartAcquireImage();
                    break;
                case "FilePick":

                    await StartFilePick();
                    break;
                case "Inking":
                    break;
            }

            return;
        }

        public async Task SetModelSource()
        {
            //CleanupCameraAsync();
            await LoadModelAsync();

            switch (_appModel.InputMedia)
            {
                case "LiveStream":
                    await ChangeLiveStream();
                    break;
                case "AcquireImage":
                    await ChangeImage();
                    break;
                case "FilePick":
                    await ChangeImage();
                    break;
                case "Inking":
                    break;
            }
        }


        private async Task StartAcquireImage()
        {

            CameraCaptureUI dialog = new CameraCaptureUI();
            dialog.PhotoSettings.AllowCropping = false;
            dialog.PhotoSettings.Format = CameraCaptureUIPhotoFormat.Png;

            StorageFile file = await dialog.CaptureFileAsync(CameraCaptureUIMode.Photo);

            if (file != null)
            {
                _appModel.InputFrame = await ImageHelper.LoadVideoFrameFromStorageFileAsync(file);
            }
            else
            {
                Debug.WriteLine("Failed to capture image");
                NotifyUser(false, "Failed to capture image.");
            }

            await ChangeImage();
        }

        public async Task StartFilePick()
        {
            Debug.WriteLine("StartFilePick");

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
                }
                await ChangeImage();
            }
            catch (Exception ex)
            {
                Debug.WriteLine($"error: {ex.Message}");
                NotifyUser(false, ex.Message);
            }
        }

        public async Task ChangeImage()
        {
            // Make sure have an input image, use default otherwise
            if (_appModel.InputFrame == null)
            {
                NotifyUser(false, "No valid image file selected, using default image instead.");
                var file = await StorageFile.GetFileFromApplicationUriAsync(new Uri($"ms-appx:///Assets/{_DefaultImageFileName}"));
                _appModel.InputFrame = await ImageHelper.LoadVideoFrameFromStorageFileAsync(file);
            }

            await EvaluateVideoFrameAsync();
        }

        private async Task EvaluateVideoFrameAsync()
        {
            Debug.WriteLine("EvaluateVideoFrameAsync");
            if (_appModel.InputFrame.Direct3DSurface != null) Debug.WriteLine("Has Direct3dsurface");
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
                NotifyUser(false, "No Camera found.");
                return;
            }

            _appModel.CameraNamesList = _mediaFrameSourceGroupList.Select(group => group.DisplayName);
        }

        public async Task ChangeLiveStream()
        {
            CleanupCameraAsync();
            Debug.WriteLine("ChangeLiveStream");
            SaveEnabled = false;

            // If webcam hasn't been initialized, bail.
            if (_mediaFrameSourceGroupList == null) return;

            try
            {
                // Check that SCI hasn't < 0 
                if (_appModel.SelectedCameraIndex < 0) _appModel.SelectedCameraIndex = 0;
                _selectedMediaFrameSourceGroup = _mediaFrameSourceGroupList[_appModel.SelectedCameraIndex];

                // Create MediaCapture and its settings
                var settings = new MediaCaptureInitializationSettings
                {
                    SourceGroup = _selectedMediaFrameSourceGroup,
                    PhotoCaptureSource = PhotoCaptureSource.Auto,
                    MemoryPreference = _appModel.UseGPU ? MediaCaptureMemoryPreference.Auto : MediaCaptureMemoryPreference.Cpu,
                    StreamingCaptureMode = StreamingCaptureMode.Video
                };
                _mediaCapture = new MediaCapture();
                await _mediaCapture.InitializeAsync(settings);

                var capture = new CaptureElement();
                capture.Source = _mediaCapture;
                _appModel.OutputCaptureElement = capture;

                // var modelPath = new Uri($"./Assets/{_appModel.ModelSource}.onnx");
                var modelPath = Path.GetFullPath($"./Assets/{_appModel.ModelSource}.onnx");
                videoEffectDefinition = new VideoEffectDefinition(_videoEffectID);
                videoEffect = await _mediaCapture.AddVideoEffectAsync(videoEffectDefinition, MediaStreamType.VideoPreview);
                videoEffect.SetProperties(new PropertySet() {
                    {"ModelName", modelPath },
                    {"UseGPU", _appModel.UseGPU }});

                await _mediaCapture.StartPreviewAsync();
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

            //_appModel.OutputCaptureElement = MediaSource.CreateFromMediaFrameSource(_selectedMediaFrameSource);
            //_appModel.InputMediaSource = MediaSource.CreateFromMediaFrameSource(_selectedMediaFrameSource);
        }

        private async Task LoadModelAsync()
        {

            m_session?.Dispose();

            StorageFile modelFile = await StorageFile.GetFileFromApplicationUriAsync(new Uri($"ms-appx:///Assets/{_appModel.ModelSource}.onnx"));
            m_model = await LearningModel.LoadFromStorageFileAsync(modelFile);

            m_inferenceDeviceSelected = _appModel.UseGPU ? LearningModelDeviceKind.DirectX : LearningModelDeviceKind.Cpu;
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

        private void CleanupCameraAsync()
        {
            Debug.WriteLine("CleanupCameraAsync");
            try
            {
                _mediaCapture?.Dispose();
                if (_appModel.OutputCaptureElement != null)
                {
                    _appModel.OutputCaptureElement = null;
                }
                if (videoEffect != null)
                {
                    videoEffect = null;
                }
                if (videoEffectDefinition != null) videoEffectDefinition = null;
            }

            catch (Exception ex)
            {
                Debug.WriteLine($"CleanupCameraAsync: {ex.Message}");
            }
        }


        private void CleanupInputImage()
        {
            try
            {
                InputSoftwareBitmapSource?.Dispose();
                OutputSoftwareBitmapSource?.Dispose();

                InputSoftwareBitmapSource = new SoftwareBitmapSource();
                OutputSoftwareBitmapSource = new SoftwareBitmapSource();

                _appModel.OutputFrame?.Dispose();
                _appModel.OutputFrame = new VideoFrame(BitmapPixelFormat.Bgra8, (int)m_outWidth, (int)m_outHeight);
            }
            catch (Exception e)
            {
                Debug.WriteLine(e.Message);
            }
        }

        public void NotifyUser(bool success, string strMessage = "")
        {
            _succeed = success;
            StatusMessage = strMessage;
        }

        public event PropertyChangedEventHandler PropertyChanged;
        private void OnPropertyChanged([CallerMemberName] string propertyName = null)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }
    }
}
