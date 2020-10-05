using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
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
using Windows.System.Display;
using Windows.UI.Core;
using GalaSoft.MvvmLight.Threading;
using System.Threading;
using StyleTransferEffectComponent;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.Devices.Enumeration;
using Microsoft.Graphics.Canvas;
using FrameSourceHelper_UWP;
using ObjectDetectorSkillSample;
using Windows.UI.Xaml;
using Windows.Foundation;

namespace StyleTransfer
{
    class AppViewModel : INotifyPropertyChanged
    {
        // Media capture properties
        public MediaCapture _mediaCapture;
        private bool _isPreviewing;
        private DisplayRequest _displayrequest = new DisplayRequest();
        DeviceInformationCollection _devices;
        private readonly object _processLock = new object();
        StyleTransferEffectComponent.StyleTransferEffectNotifier _notifier;

        // Style transfer effect properties
        private LearningModel _learningModel = null;
        private LearningModelDeviceKind _inferenceDeviceSelected = LearningModelDeviceKind.Default;
        private LearningModelSession _session;
        private LearningModelBinding _binding;
        private string _inputImageDescription;
        private string _outputImageDescription;
        // Activatable Class ID of the video effect. 
        private const string StyleTransferEffectId = "StyleTransferEffectComponent.StyleTransferEffect";

        // Image style transfer properties
        uint _inWidth, _inHeight, _outWidth, _outHeight;
        private const string DefaultImageFileName = "Input.jpg";

        // User notifiaction properties
        private bool _succeed;
        private readonly SolidColorBrush _successColor = new SolidColorBrush(Windows.UI.Colors.Green);
        private readonly SolidColorBrush _failColor = new SolidColorBrush(Windows.UI.Colors.Red);

        // Locks
        private SemaphoreSlim m_lock = new SemaphoreSlim(1);
        private MediaPlayerFrameSource m_frameSource = null;
        private VideoFrame m_renderTargetFrame = null;
        private uint m_frameCounter = 0;
        private uint m_maxNumberTrackers = 5;
        private uint m_maxTrackerHistoryLength = 20;
        private uint m_detectorEvalInterval = 150;
        private BoundingBoxRenderer m_renderer = null;

        string[] labels = new string[80]
        {
            "person",
            "bicycle",
            "car",
            "motorbike",
            "aeroplane",
            "bus",
            "train",
            "truck",
            "boat",
            "traffic light",
            "fire hydrant",
            "stop sign",
            "parking meter",
            "bench",
            "bird",
            "cat",
            "dog",
            "horse",
            "sheep",
            "cow",
            "elephant",
            "bear",
            "zebra",
            "giraffe",
            "backpack",
            "umbrella",
            "handbag",
            "tie",
            "suitcase",
            "frisbee",
            "skis",
            "snowboard",
            "sports ball",
            "kite",
            "baseball bat",
            "baseball glove",
            "skateboard",
            "surfboard",
            "tennis racket",
            "bottle",
            "wine glass",
            "cup",
            "fork",
            "knife",
            "spoon",
            "bowl",
            "banana",
            "apple",
            "sandwich",
            "orange",
            "broccoli",
            "carrot",
            "hot dog",
            "pizza",
            "donut",
            "cake",
            "chair",
            "sofa",
            "potted plant",
            "bed",
            "dining table",
            "toilet",
            "tvmonitor",
            "laptop",
            "mouse",
            "remote",
            "keyboard",
            "cell phone",
            "microwave",
            "oven",
            "toaster",
            "sink",
            "refrigerator",
            "book",
            "clock",
            "vase",
            "scissors",
            "teddy bear",
            "hair drier",
            "toothbrush"
        };

        class Comparer : IComparer<DetectionResult>
        {
            public int Compare(DetectionResult x, DetectionResult y)
            {
                if (x.prob > y.prob)
                {
                    return 0;
                }
                else
                {
                    return 1;
                }
            }
        }
        public AppViewModel()
        {
            _appModel = new AppModel();
            _appModel.PropertyChanged += _appModel_PropertyChanged;

            SaveCommand = new RelayCommand(SaveOutput);
            SetMediaSourceCommand = new RelayCommand<InputMediaType>((x) => _appModel.InputMedia = x);
            SetModelSourceCommand = new RelayCommand(async () => await SetModelSource());
            ChangeLiveStreamCommand = new RelayCommand(async () => await ChangeLiveStream());

            _inputSoftwareBitmapSource = new SoftwareBitmapSource();
            _outputSoftwareBitmapSource = new SoftwareBitmapSource();
            _UIOverlayCanvas = new Canvas();
            _UIOverlayCanvas.Width = 1280;
            _UIOverlayCanvas.Height = 920;
            _saveEnabled = true;

            _notifier = new StyleTransferEffectComponent.StyleTransferEffectNotifier();
            _notifier.FrameRateUpdated += async (_, e) =>
            {
                await DispatcherHelper.RunAsync(() =>
                {
                    RenderFPS = e;
                });

            };
            UseGpu = false;
            _isPreviewing = false;
            NumThreads = 5;
            NotifyUser(true);
            // Initialize helper class used to render the skill results on screen
            m_renderer = new BoundingBoxRenderer(UIOverlayCanvas);
            // m_detections = new List<DetectionResult>();

        }

        private AppModel _appModel;
        public AppModel AppModel
        {
            get
            {
                return _appModel;
            }
        }

        private int _numThreads;
        public int NumThreads
        {
            get
            {
                return _numThreads;
            }
            set
            {
                _numThreads = value;
                OnPropertyChanged();
            }
        }
        public int MaxThreads
        {
            get
            {
                return 10;
            }
        }

        private float _renderFPS;
        public float RenderFPS
        {
            get
            {
                return (float)Math.Round(_renderFPS, 2);
            }
            set
            {
                _renderFPS = value;
                OnPropertyChanged();
            }
        }

        private float _captureFPS;
        public float CaptureFPS
        {
            get
            {
                return _captureFPS;
            }
            set
            {
                _captureFPS = value;
                OnPropertyChanged();
            }
        }

        private SoftwareBitmapSource _inputSoftwareBitmapSource;
        public SoftwareBitmapSource InputSoftwareBitmapSource
        {
            get
            {
                return _inputSoftwareBitmapSource;
            }
            set
            {
                _inputSoftwareBitmapSource = value;
                OnPropertyChanged();
            }
        }

        private SoftwareBitmapSource _outputSoftwareBitmapSource;
        public SoftwareBitmapSource OutputSoftwareBitmapSource
        {
            get
            {
                return _outputSoftwareBitmapSource;
            }
            set
            {
                _outputSoftwareBitmapSource = value;
                OnPropertyChanged();
            }
        }

        private Canvas _UIOverlayCanvas;
        public Canvas UIOverlayCanvas
        {
            get
            {
                return _UIOverlayCanvas;
            }
            set
            {
                _UIOverlayCanvas = value;
                OnPropertyChanged();
            }
        }

        private bool _saveEnabled;
        public bool SaveEnabled
        {
            get
            {
                return _saveEnabled;
            }
            set
            {
                _saveEnabled = value;
                OnPropertyChanged();
            }
        }

        public SolidColorBrush StatusBarColor
        {
            get
            {
                return _succeed ? _successColor : _failColor;
            }
        }

        private string _statusMessage;
        public string StatusMessage
        {
            get
            {
                return _statusMessage;
            }
            set
            {
                _statusMessage = value;
                OnPropertyChanged();
                OnPropertyChanged("StatusBarColor");
            }
        }

        private bool _useGpu;
        public bool UseGpu
        {
            get
            {
                return _useGpu;
            }
            set
            {
                _useGpu = value;
                if (_appModel.InputMedia == InputMediaType.LiveStream)
                {
                    SetMediaSourceCommand.Execute(InputMediaType.LiveStream);
                }
                OnPropertyChanged();
            }
        }

        // Command defintions
        public ICommand SaveCommand { get; set; }
        public ICommand SetMediaSourceCommand { get; set; }
        public ICommand ChangeLiveStreamCommand { get; set; }
        public ICommand SetModelSourceCommand { get; set; }

        private async void _appModel_PropertyChanged(object sender, PropertyChangedEventArgs e)
        {
            Debug.WriteLine(e.PropertyName);
            if (e.PropertyName == "InputMedia")
            {
                await SetMediaSource();
            }
        }

        public async void SaveOutput()
        {
            Debug.WriteLine("UIButtonSaveImage_Click");
            if (_appModel.InputMedia != InputMediaType.LiveStream)
            {
                await ImageHelper.SaveVideoFrameToFilePickedAsync(_appModel.OutputFrame);
            }
        }

        public async Task SetMediaSource()
        {
            await LoadModelAsync();
            await CleanupCameraAsync();
            CleanupInputImage();

            NotifyUser(true);
            SaveEnabled = true;

            switch (_appModel.InputMedia)
            {
                case InputMediaType.LiveStream:
                    await StartLiveStream();
                    break;
                case InputMediaType.AcquireImage:
                    await StartAcquireImage();
                    break;
                case InputMediaType.FilePick:
                    await StartFilePick();
                    break;
                case InputMediaType.VideoPick:
                    await StartVideoPick();
                    break;
                default:
                    break;
            }
            return;
        }

        public async Task SetModelSource()
        {
            Debug.WriteLine("SetModelSource");
            await LoadModelAsync();

            switch (_appModel.InputMedia)
            {
                case InputMediaType.LiveStream:
                    await ChangeLiveStream();
                    break;
                case InputMediaType.AcquireImage:
                    await ChangeImage();
                    break;
                case InputMediaType.FilePick:
                    await ChangeImage();
                    break;
                case InputMediaType.VideoPick:
                    await ChangeImage();
                    break;
                default:
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
                    NotifyUser(false, "No valid image file selected, using default image instead.");
                    var file = await StorageFile.GetFileFromApplicationUriAsync(new Uri($"ms-appx:///Assets/{DefaultImageFileName}"));
                    _appModel.InputFrame = await ImageHelper.LoadVideoFrameFromStorageFileAsync(file);
                }
                await ChangeImage();
            }
            catch (Exception ex)
            {
                Debug.WriteLine($"error: {ex.Message}");
                NotifyUser(false, ex.Message);
            }
        }

        public async Task StartVideoPick()
        {
            StorageFile videoPath = await ImageHelper.GetSelectedStorageFile();
            
            await GetStream(videoPath);
        }

        private async Task GetStream(StorageFile videoPath)
        {
            await m_lock.WaitAsync();
            {
                // Clean up previous frame source
                if (m_frameSource != null)
                {
                    m_frameSource.FrameArrived -= FrameSource_FrameAvailable;
                    var disposableFrameSource = m_frameSource as IDisposable;
                    if (disposableFrameSource != null)
                    {
                        // Lock disposal based on frame source consumers
                        disposableFrameSource.Dispose();
                    }
                }

                // Create new frame source and register a callback if the source fails along the way
                m_frameSource = await MediaPlayerFrameSource.CreateFromStorageFileAsyncTask(
                    videoPath,
                    (sender, message) =>
                    {
                        NotifyUser(false, message);
                    });
                // Clear existing trackers
                m_frameCounter = 0;

            }
            m_lock.Release();

            // If we obtained a valid frame source, start it
            if (m_frameSource != null)
            {
                m_frameSource.FrameArrived += FrameSource_FrameAvailable;
                await m_frameSource.StartAsync();
            }
        }

        private void FrameSource_FrameAvailable(object sender, VideoFrame frame)
        {
            // Locking behavior, so only one skill execution happens at a time
            if (m_lock.Wait(0))
            {
#pragma warning disable CS4014
                // Purposely don't await this: want handler to exit ASAP
                // so that realtime capture doesn't wait for completion.
                // Instead, we unlock only when processing finishes ensuring that
                // only one execution is active at a time, dropping frames or
                // aborting skill runs as necessary
                Task.Run(async () =>
                {
                    try
                    {
                        // Evaluate()
                        // _appModel.InputFrame = frame;
                        IReadOnlyList<DetectionResult> detectedObjects = DetectObjectsAsync(frame);
                        await DisplayFrameAndResultAsync(frame, detectedObjects);
                    }
                    catch (Exception ex)
                    {
                        NotifyUser(false, ex.Message);
                    }
                    finally
                    {
                        m_lock.Release();
                    }
                });
#pragma warning restore CS4014
            }
        }

        private IReadOnlyList<DetectionResult> DetectObjectsAsync(VideoFrame frame)
        {

            LearningModelEvaluationResult results;
            lock (_processLock)
            {
                _binding.Bind(_inputImageDescription, ImageFeatureValue.CreateFromVideoFrame(frame));
                // _binding.Bind(_outputImageDescription, ImageFeatureValue.CreateFromVideoFrame(_appModel.OutputFrame));
                results = _session.Evaluate(_binding, "test");
            }
            return ParseOutput(results);
        }

        private List<DetectionResult> ParseOutput(LearningModelEvaluationResult results)
        {
            List<DetectionResult> detections = new List<DetectionResult>();
            var resultTensor = results.Outputs[_outputImageDescription] as TensorFloat;
            var resultVector = resultTensor.GetAsVectorView();
            var resultShape = resultTensor.Shape;

            for (int i = 0; i < resultVector.Count / 84; i++)
            {
                Debug.Write($"{i}\n");
                if (resultVector.ElementAt(84 * i) < 0 ||
                    resultVector.ElementAt(84 * i + 1) < 0 ||
                    resultVector.ElementAt(84 * i + 2) < 0 ||
                    resultVector.ElementAt(84 * i + 3) < 0)
                {
                    continue;
                }
                //Rect clampedRect = new Rect(
                //    new Point(resultVector.ElementAt(84 * i), resultVector.ElementAt(84 * i + 1)),
                //    new Point(resultVector.ElementAt(84 * i + 2), resultVector.ElementAt(84 * i + 3)));
                Rect clampedRect = new Rect();
                clampedRect.X = resultVector.ElementAt(84 * i);
                clampedRect.Y = resultVector.ElementAt(84 * i + 1);
                clampedRect.Width = resultVector.ElementAt(84 * i + 2);
                clampedRect.Height = resultVector.ElementAt(84 * i + 3);

                float max_prob = 0;
                int max_index = -1;
                for (int j = 0; j < 80; ++j)
                {
                    if (resultVector.ElementAt(84 * i + 4 + j) > max_prob)
                    {
                        max_prob = resultVector.ElementAt(84 * i + 4 + j);
                        max_index = j;
                    }
                }
                if (max_index != 0 && max_index != 6)
                {
                    continue;
                }
                Debug.Write($"{i} : {labels[max_index]}\n");
                detections.Add(new DetectionResult()
                {
                    label = labels[max_index],
                    bbox = clampedRect,
                    prob = max_prob
                });

            }
            Comparer cp = new Comparer();
            detections.Sort(cp);
            return detections;
        }
        private async Task DisplayFrameAndResultAsync(VideoFrame frame, IReadOnlyList<DetectionResult> detectedObjects)
        {
            await DispatcherHelper.RunAsync(async () =>
            {
                try
                {
                    SoftwareBitmap targetSoftwareBitmap = frame.SoftwareBitmap;

                    // If we receive a Direct3DSurface-backed VideoFrame, convert to a SoftwareBitmap in a format that can be rendered via the UI element
                    if (targetSoftwareBitmap == null)
                    {
                        if (m_renderTargetFrame == null)
                        {
                            m_renderTargetFrame = new VideoFrame(BitmapPixelFormat.Bgra8, frame.Direct3DSurface.Description.Width, frame.Direct3DSurface.Description.Height, BitmapAlphaMode.Ignore);
                        }

                        // Leverage the VideoFrame.CopyToAsync() method that can convert the input Direct3DSurface-backed VideoFrame to a SoftwareBitmap-backed VideoFrame
                        await frame.CopyToAsync(m_renderTargetFrame);
                        targetSoftwareBitmap = m_renderTargetFrame.SoftwareBitmap;
                    }
                    // Else, if we receive a SoftwareBitmap-backed VideoFrame, if its format cannot already be rendered via the UI element, convert it accordingly
                    else
                    {
                        if (targetSoftwareBitmap.BitmapPixelFormat != BitmapPixelFormat.Bgra8 || targetSoftwareBitmap.BitmapAlphaMode != BitmapAlphaMode.Ignore)
                        {
                            if (m_renderTargetFrame == null)
                            {
                                m_renderTargetFrame = new VideoFrame(BitmapPixelFormat.Bgra8, targetSoftwareBitmap.PixelWidth, targetSoftwareBitmap.PixelHeight, BitmapAlphaMode.Ignore);
                            }

                            // Leverage the VideoFrame.CopyToAsync() method that can convert the input SoftwareBitmap-backed VideoFrame to a different format
                            await frame.CopyToAsync(m_renderTargetFrame);
                            targetSoftwareBitmap = m_renderTargetFrame.SoftwareBitmap;
                        }
                    }
                    // await m_processedBitmapSource.SetBitmapAsync(targetSoftwareBitmap);
                    _appModel.InputFrame = VideoFrame.CreateWithSoftwareBitmap(targetSoftwareBitmap);
                    await InputSoftwareBitmapSource.SetBitmapAsync(targetSoftwareBitmap);

                    // Lock so eval + binding not destroyed mid-evaluation
                    //Debug.Write("Eval Begin | ");
                    //LearningModelEvaluationResult results;
                    //lock (_processLock)
                    //{
                    //    Debug.Write("Eval Lock | ");
                    //    _binding.Bind(_inputImageDescription, ImageFeatureValue.CreateFromVideoFrame(_appModel.InputFrame));
                    //    // _binding.Bind(_outputImageDescription, ImageFeatureValue.CreateFromVideoFrame(_appModel.OutputFrame));
                    //    results = _session.Evaluate(_binding, "test");
                    //}
                    //Debug.Write("Eval Unlock\n");
                    
                    // Parse Results
                    //IReadOnlyDictionary<string, object> outputs = results.Outputs;
                    //foreach (var output in outputs)
                    //{
                    //    Debug.WriteLine($"{output.Key} : {output.Value} -> {output.Value.GetType()}");
                    //}
                    // await OutputSoftwareBitmapSource.SetBitmapAsync(_appModel.OutputFrame.SoftwareBitmap);
                    //m_renderer.ClearCanvas();
                    m_renderer.Render(detectedObjects);
                }
                catch (TaskCanceledException)
                {
                    // no-op: we expect this exception when we change media sources
                    // and can safely ignore/continue
                }
                catch (Exception ex)
                {
                    NotifyUser(false, $"Exception while rendering results: {ex.Message}");
                }
            });
        }

        public async Task ChangeImage()
        {
            // Make sure have an input image, use default otherwise
            if (_appModel.InputFrame == null)
            {
                NotifyUser(false, "No valid image file selected, using default image instead.");
                var file = await StorageFile.GetFileFromApplicationUriAsync(new Uri($"ms-appx:///Assets/{DefaultImageFileName}"));
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
                _appModel.InputFrame = await ImageHelper.CenterCropImageAsync(_appModel.InputFrame, _inWidth, _inHeight);
                await InputSoftwareBitmapSource.SetBitmapAsync(_appModel.InputFrame.SoftwareBitmap);

                // Lock so eval + binding not destroyed mid-evaluation
                Debug.Write("Eval Begin | ");
                LearningModelEvaluationResult results;
                lock (_processLock)
                {
                    Debug.Write("Eval Lock | ");
                    _binding.Bind(_inputImageDescription, ImageFeatureValue.CreateFromVideoFrame(_appModel.InputFrame));
                    _binding.Bind(_outputImageDescription, ImageFeatureValue.CreateFromVideoFrame(_appModel.OutputFrame));
                    results = _session.Evaluate(_binding, "test");
                }
                Debug.Write("Eval Unlock\n");

                // Parse Results
                IReadOnlyDictionary<string, object> outputs = results.Outputs;
                foreach (var output in outputs)
                {
                    Debug.WriteLine($"{output.Key} : {output.Value} -> {output.Value.GetType()}");
                }

                await OutputSoftwareBitmapSource.SetBitmapAsync(_appModel.InputFrame.SoftwareBitmap);
            }
        }

        private async Task EvaluateVideoFrameAsync_2(VideoFrame frame)
        {
            await OutputSoftwareBitmapSource.SetBitmapAsync(frame.SoftwareBitmap);
        }

        public async Task StartLiveStream()
        {
            Debug.WriteLine("StartLiveStream");
            await CleanupCameraAsync();

            if (_devices == null)
            {
                try
                {
                    _devices = await DeviceInformation.FindAllAsync(DeviceClass.VideoCapture);
                }
                catch (Exception ex)
                {
                    Debug.WriteLine(ex.Message);
                    _devices = null;
                }
                if ((_devices == null) || (_devices.Count == 0))
                {
                    // No camera sources found
                    Debug.WriteLine("No cameras found");
                    NotifyUser(false, "No cameras found.");
                    return;
                }
                _appModel.CameraNamesList = _devices.Select(device => device.Name);
                return;
            }
            // If just above 0 you fool
            if (_appModel.SelectedCameraIndex >= 0)
            {
                Debug.WriteLine("StartLive: already 0");
                await ChangeLiveStream();
                return;
            }
            // If already have the list, set to the default
            _appModel.SelectedCameraIndex = 0;
        }

        public async Task ChangeLiveStream()
        {
            Debug.WriteLine("ChangeLiveStream");
            await CleanupCameraAsync();

            SaveEnabled = false;

            // If webcam hasn't been initialized, bail.
            if ((_devices == null) || (_devices.Count == 0))
                return;

            try
            {
                // Check that SCI hasn't < 0 
                // Probably -1 when doesn't find camera, or when list gone? 
                if (_appModel.SelectedCameraIndex < 0)
                {
                    Debug.WriteLine("selectedCamera < 0");
                    NotifyUser(false, "Invalid Camera selected, using default");
                    _appModel.SelectedCameraIndex = 0;
                    return;
                }
                var device = _devices.ToList().ElementAt(_appModel.SelectedCameraIndex);
                // Create MediaCapture and its settings
                var settings = new MediaCaptureInitializationSettings
                {
                    VideoDeviceId = device.Id,
                    PhotoCaptureSource = PhotoCaptureSource.Auto,
                    MemoryPreference = UseGpu ? MediaCaptureMemoryPreference.Auto : MediaCaptureMemoryPreference.Cpu,
                    StreamingCaptureMode = StreamingCaptureMode.Video,
                    MediaCategory = MediaCategory.Communications,
                };
                _mediaCapture = new MediaCapture();
                await _mediaCapture.InitializeAsync(settings);
                _displayrequest.RequestActive();

                var capture = new CaptureElement();
                capture.Source = _mediaCapture;
                _appModel.OutputCaptureElement = capture;

                var modelPath = Path.GetFullPath($"./Assets/{_appModel.ModelSource}.onnx");
                VideoEffectDefinition videoEffectDefinition = new VideoEffectDefinition(StyleTransferEffectId, new PropertySet() {
                    {"ModelName", modelPath },
                    {"UseGpu", UseGpu },
                    { "Notifier", _notifier},
                    { "NumThreads", NumThreads} });
                IMediaExtension videoEffect = await _mediaCapture.AddVideoEffectAsync(videoEffectDefinition, MediaStreamType.VideoPreview);

                var props = _mediaCapture.VideoDeviceController.GetMediaStreamProperties(MediaStreamType.VideoPreview) as VideoEncodingProperties;
                CaptureFPS = props.FrameRate.Numerator / props.FrameRate.Denominator;

                await _mediaCapture.StartPreviewAsync();
                _isPreviewing = true;
            }
            catch (Exception ex)
            {
                Debug.WriteLine(ex.ToString());
            }
        }


        private void _mediaCapture_Failed(MediaCapture sender, MediaCaptureFailedEventArgs errorEventArgs)
        {
            Debug.WriteLine("_mediaCapture FAIL! " + errorEventArgs.Message);
        }

        private async Task LoadModelAsync()
        {
            Debug.Write("LoadModelBegin | ");

            Debug.Write("LoadModel Lock | ");

            _binding?.Clear();
            _session?.Dispose();

            StorageFile modelFile = await StorageFile.GetFileFromApplicationUriAsync(new Uri($"ms-appx:///Assets/{_appModel.ModelSource}.onnx"));
            _learningModel = await LearningModel.LoadFromStorageFileAsync(modelFile);
            _inferenceDeviceSelected = UseGpu ? LearningModelDeviceKind.DirectX : LearningModelDeviceKind.Cpu;

            // Lock so can't create a new session or binding while also being disposed
            lock (_processLock)
            {
                _session = new LearningModelSession(_learningModel, new LearningModelDevice(_inferenceDeviceSelected));
                _binding = new LearningModelBinding(_session);
            }

            debugModelIO();
            _inputImageDescription = _learningModel.InputFeatures.ToList().First().Name;
            _outputImageDescription = _learningModel.OutputFeatures.ToList().First().Name;

            Debug.Write("LoadModel Unlock\n");
        }

        public void debugModelIO()
        {
            string _inName; // _outName;
            foreach (var inputF in _learningModel.InputFeatures)
            {
                Debug.WriteLine($"input | kind:{inputF.Kind}, name:{inputF.Name}, type:{inputF.GetType()}");
#pragma warning disable CS0219 // The variable 'i' is assigned but its value is never used
                int i = 0;
#pragma warning restore CS0219 // The variable 'i' is assigned but its value is never used
                ImageFeatureDescriptor imgDesc = inputF as ImageFeatureDescriptor;
                TensorFeatureDescriptor tfDesc = inputF as TensorFeatureDescriptor;
                _inWidth = (uint)(imgDesc == null ? tfDesc.Shape[3] : imgDesc.Width);
                _inHeight = (uint)(imgDesc == null ? tfDesc.Shape[2] : imgDesc.Height);
                _inName = inputF.Name;

                Debug.WriteLine($"N: {(imgDesc == null ? tfDesc.Shape[0] : 1)}, " +
                    $"Channel: {(imgDesc == null ? tfDesc.Shape[1].ToString() : imgDesc.BitmapPixelFormat.ToString())}, " +
                    $"Height:{(imgDesc == null ? tfDesc.Shape[2] : imgDesc.Height)}, " +
                    $"Width: {(imgDesc == null ? tfDesc.Shape[3] : imgDesc.Width)}");
            }
//            foreach (var outputF in _learningModel.OutputFeatures)
//            {
//                Debug.WriteLine($"output | kind:{outputF.Kind}, name:{outputF.Name}, type:{outputF.GetType()}");
//#pragma warning disable CS0219 // The variable 'i' is assigned but its value is never used
//                int i = 0;
//#pragma warning restore CS0219 // The variable 'i' is assigned but its value is never used
//                ImageFeatureDescriptor imgDesc = outputF as ImageFeatureDescriptor;
//                TensorFeatureDescriptor tfDesc = outputF as TensorFeatureDescriptor;
//                _outWidth = (uint)(imgDesc == null ? tfDesc.Shape[3] : imgDesc.Width);
//                _outHeight = (uint)(imgDesc == null ? tfDesc.Shape[2] : imgDesc.Height);
//                _outName = outputF.Name;

//                Debug.WriteLine($"N: {(imgDesc == null ? tfDesc.Shape[0] : 1)}, " +
//                   $"Channel: {(imgDesc == null ? tfDesc.Shape[1].ToString() : imgDesc.BitmapPixelFormat.ToString())}, " +
//                   $"Height:{(imgDesc == null ? tfDesc.Shape[2] : imgDesc.Height)}, " +
//                   $"Width: {(imgDesc == null ? tfDesc.Shape[3] : imgDesc.Width)}");
//            }
        }

        private async Task CleanupCameraAsync()
        {
            Debug.WriteLine("CleanupCameraAsync");
            try
            {
                RenderFPS = 0;
                // Clean up the media capture
                if (_mediaCapture != null)
                {
                    if (_isPreviewing)
                    {
                        try
                        {
                            await _mediaCapture.StopPreviewAsync();
                        }
                        catch (Exception e) { Debug.WriteLine("CleanupCamera: " + e.Message); }
                    }
                    await DispatcherHelper.RunAsync(() =>
                    {
                        CaptureElement cap = new CaptureElement();
                        cap.Source = null;
                        _appModel.OutputCaptureElement = cap;
                        if (_isPreviewing)
                        {
                            _displayrequest.RequestRelease();
                        }

                        MediaCapture m = _mediaCapture;
                        _mediaCapture = null;
                        m.Dispose();
                        _isPreviewing = false;
                    });
                }

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
                if (InputSoftwareBitmapSource != null)
                {
                    InputSoftwareBitmapSource.Dispose();
                    InputSoftwareBitmapSource = new SoftwareBitmapSource();
                }
                if (OutputSoftwareBitmapSource != null)
                {
                    OutputSoftwareBitmapSource.Dispose();
                    OutputSoftwareBitmapSource = new SoftwareBitmapSource();
                }
                if (_appModel.OutputFrame != null)
                {
                    _appModel.OutputFrame.Dispose();
                }
                _appModel.OutputFrame = new VideoFrame(BitmapPixelFormat.Bgra8, (int)_outWidth, (int)_outHeight);

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
