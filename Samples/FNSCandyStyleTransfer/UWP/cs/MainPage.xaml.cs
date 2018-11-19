//*@@@+++@@@@******************************************************************
//
// Microsoft Windows Media Foundation
// Copyright (C) Microsoft Corporation. All rights reserved.
//
//*@@@---@@@@******************************************************************

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;
using Windows.AI.MachineLearning;
using Windows.Foundation;
using Windows.Graphics.DirectX.Direct3D11;
using Windows.Graphics.Imaging;
using Windows.Media;
using Windows.Media.Capture;
using Windows.Media.Capture.Frames;
using Windows.Media.Core;
using Windows.Media.MediaProperties;
using Windows.Media.Playback;
using Windows.Storage;
using Windows.UI.Core;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Navigation;
using Windows.UI.Xaml.Media.Imaging;

namespace SnapCandy
{
    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class MainPage : Page
    {
        // Camera related
        private MediaCapture _mediaCapture;
        private MediaPlayer _mediaPlayer;
        private List<MediaFrameSourceGroup> _mediaFrameSourceGroupList;
        private MediaFrameSourceGroup _selectedMediaFrameSourceGroup;
        private MediaFrameSource _selectedMediaFrameSource;
        private MediaFrameReader _modelInputFrameReader;

        // States
        private bool _isProcessingFrames = false;
        private bool _showInitialImageAndProgress = true;
        private SemaphoreSlim _frameAquisitionLock = new SemaphoreSlim(1);
        private bool _useGPU = true;
        private DispatcherTimer _inkEvaluationDispatcherTimer;
        private bool _isrocessingImages = true;

        // Rendering related
        private FrameRenderer _resultframeRenderer;
        private FrameRenderer _inputFrameRenderer;

        // WinML related
        private readonly List<string> _kModelFileNames = new List<string>
        {
            "winmlperf_coreml_FNS-Candy_prerelease",
            "la_muse",
            "rain_princess",
            "udnie",
            "wave"
        };
        private const string _kDefaultImageFileName = "DefaultImage.jpg";
        private LearningModel m_model = null;
        private LearningModelDeviceKind m_inferenceDeviceSelected = LearningModelDeviceKind.Default;
        private LearningModelDevice m_device;
        private LearningModelSession m_session;
        uint _outWidth, _outHeight, _inWidth, _inHeight;
        string m_outName, m_inName;
        private List<string> _labels = new List<string>();
        object _nextFrame = null;  // can be a MediaFrameReference or a VideoFrame
        VideoFrame _outputFrame = null;

        // Debug
        private Stopwatch _perfStopwatch = new Stopwatch(); // performance Stopwatch used throughout
        private DispatcherTimer _FramesPerSecondTimer = new DispatcherTimer();
        private long _FramesPerSecond = 0;

        private Thread _EvaluateThread;

        public MainPage()
        {
            this.InitializeComponent();
            // Create our evaluate thread task
            _EvaluateThread = new Thread(new ParameterizedThreadStart(EvaluateThreadProc));
            _EvaluateThread.Start(this);
        }

        protected override void OnNavigatedTo(NavigationEventArgs e)
        {
            Debug.WriteLine("OnNavigatedTo");
            UIStyleList.ItemsSource = _kModelFileNames;

            UIInkCanvasInput.InkPresenter.InputDeviceTypes =
               CoreInputDeviceTypes.Mouse
               | CoreInputDeviceTypes.Pen
               | CoreInputDeviceTypes.Touch;

            UIInkCanvasInput.InkPresenter.UpdateDefaultDrawingAttributes(
                new Windows.UI.Input.Inking.InkDrawingAttributes()
                {
                    Color = Windows.UI.Colors.Black,
                    Size = new Size(8, 8),
                    IgnorePressure = true,
                    IgnoreTilt = true,
                }
            );

            // Create a 1 second timer
            _FramesPerSecondTimer.Tick += _FramesPerSecond_Tick;
            _FramesPerSecondTimer.Interval = new TimeSpan(0, 0, 1);
            _FramesPerSecondTimer.Start();

            // Select first style
            UIStyleList.SelectedIndex = 0;
        }

        private void _FramesPerSecond_Tick(object sender, object e)
        {
            // how many frames did we present?
            long intervalFramesPerSecond = _FramesPerSecond;
            _FramesPerSecond = 0;

            NotifyUser($"FramesPerSecond: {intervalFramesPerSecond}", NotifyType.StatusMessage);
        }

        /// <summary>
        /// Display a message to the user.
        /// This method may be called from any thread.
        /// </summary>
        /// <param name="strMessage"></param>
        /// <param name="type"></param>
        public void NotifyUser(string strMessage, NotifyType type)
        {
            // If called from the UI thread, then update immediately.
            // Otherwise, schedule a task on the UI thread to perform the update.
            if (Dispatcher.HasThreadAccess)
            {
                UpdateStatus(strMessage, type);
            }
            else
            {
                var task = Dispatcher.RunAsync(CoreDispatcherPriority.Normal, () => UpdateStatus(strMessage, type));
                task.AsTask().Wait();
            }
        }

        /// <summary>
        /// Update the status message displayed on the UI
        /// </summary>
        /// <param name="strMessage"></param>
        /// <param name="type"></param>
        private void UpdateStatus(string strMessage, NotifyType type)
        {
            switch (type)
            {
                case NotifyType.StatusMessage:
                    UIStatusBorder.Background = new SolidColorBrush(Windows.UI.Colors.Green);
                    break;
                case NotifyType.ErrorMessage:
                    UIStatusBorder.Background = new SolidColorBrush(Windows.UI.Colors.Red);
                    break;
            }

            StatusBlock.Text = strMessage;

            // Collapse the StatusBlock if it has no text to conserve real estate.
            UIStatusBorder.Visibility = (StatusBlock.Text != String.Empty) ? Visibility.Visible : Visibility.Collapsed;
            if (StatusBlock.Text != String.Empty)
            {
                UIStatusBorder.Visibility = Visibility.Visible;
                UIStatusPanel.Visibility = Visibility.Visible;
            }
            else
            {
                UIStatusBorder.Visibility = Visibility.Collapsed;
                UIStatusPanel.Visibility = Visibility.Collapsed;
            }
        }

        /// <summary>
        /// Load the labels and model and initialize WinML
        /// </summary>
        /// <returns></returns>
        private async Task LoadModelAsync(string modelFileName)
        {
            Debug.WriteLine("LoadModelAsync");
            {
                m_model = null;
                m_session = null;
                try
                {
                    // Start stopwatch
                    _perfStopwatch.Restart();

                    // Load Model
                    StorageFile modelFile = await StorageFile.GetFileFromApplicationUriAsync(new Uri($"ms-appx:///Assets/{modelFileName}.onnx"));
                    m_model = await LearningModel.LoadFromStorageFileAsync(modelFile);

                    // Stop stopwatch
                    _perfStopwatch.Stop();

                    // Setting preferred inference device given user's intent
                    m_inferenceDeviceSelected = _useGPU ? LearningModelDeviceKind.DirectXHighPerformance : LearningModelDeviceKind.Cpu;
                    m_session = new LearningModelSession(m_model, new LearningModelDevice(m_inferenceDeviceSelected));

                    // Debugging logic to see the input and output of ther model and retrieve dimensions of input/output variables
                    // ### DEBUG ###
                    foreach (var inputF in m_model.InputFeatures)
                    {
                        Debug.WriteLine($"input | kind:{inputF.Kind}, name:{inputF.Name}, type:{inputF.GetType()}");
                        int i = 0;
                        ImageFeatureDescriptor imgDesc = inputF as ImageFeatureDescriptor;
                        TensorFeatureDescriptor tfDesc = inputF as TensorFeatureDescriptor;
                        _inWidth = (uint)(imgDesc == null ? tfDesc.Shape[3] : imgDesc.Width);
                        _inHeight = (uint)(imgDesc == null ? tfDesc.Shape[2] : imgDesc.Height);
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
                        _outWidth = (uint)(imgDesc == null ? tfDesc.Shape[3] : imgDesc.Width);
                        _outHeight = (uint)(imgDesc == null ? tfDesc.Shape[2] : imgDesc.Height);
                        m_outName = outputF.Name;

                        Debug.WriteLine($"N: {(imgDesc == null ? tfDesc.Shape[0] : 1)}, " +
                           $"Channel: {(imgDesc == null ? tfDesc.Shape[1].ToString() : imgDesc.BitmapPixelFormat.ToString())}, " +
                           $"Height:{(imgDesc == null ? tfDesc.Shape[2] : imgDesc.Height)}, " +
                           $"Width: {(imgDesc == null ? tfDesc.Shape[3] : imgDesc.Width)}");
                    }
                    // ### END OF DEBUG ###

                    Debug.WriteLine($"Elapsed time: {_perfStopwatch.ElapsedMilliseconds} ms");
                }
                catch (Exception ex)
                {
                    NotifyUser($"error: {ex.Message}", NotifyType.ErrorMessage);
                    Debug.WriteLine($"error: {ex.Message}");
                }
            }
        }

        /// <summary>
        /// Acquire manually an image from the camera preview stream
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private async void UIButtonAcquireImage_Click(object sender, RoutedEventArgs e)
        {
            Debug.WriteLine("UIButtonAcquireImage_Click");
            {
                await CleanupCameraAsync();
                CleanupInk();
            }

            UIInputImage.Visibility = Visibility.Visible;
            _showInitialImageAndProgress = true;
            UIImageControls.IsEnabled = false;
            UIModelControls.IsEnabled = false;

            CameraCaptureUI dialog = new CameraCaptureUI();
            dialog.PhotoSettings.AllowCropping = false;
            dialog.PhotoSettings.Format = CameraCaptureUIPhotoFormat.Png;

            StorageFile file = await dialog.CaptureFileAsync(CameraCaptureUIMode.Photo);
            if (file != null)
            {
                var vf = await ImageHelper.LoadVideoFrameFromStorageFileAsync(file);
                await Task.Run(() =>
                {
                    EvaluateVideoFrame(vf);
                });
            }

            UIImageControls.IsEnabled = true;
            UIModelControls.IsEnabled = true;
        }

        /// <summary>
        /// Select and evaluate a picture
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private async void UIButtonFilePick_Click(object sender, RoutedEventArgs e)
        {
            Debug.WriteLine("UIButtonFilePick_Click");
            {
                await CleanupCameraAsync();
                CleanupInk();
            }

            UIInputImage.Visibility = Visibility.Visible;
            _showInitialImageAndProgress = true;
            _isrocessingImages = true;
            UIImageControls.IsEnabled = false;
            UIModelControls.IsEnabled = false;
            try
            {
                VideoFrame inputFrame = null;
                // use a default image
                if (sender == null && e == null)
                {
                    var file = await StorageFile.GetFileFromApplicationUriAsync(new Uri($"ms-appx:///Assets/{_kDefaultImageFileName}"));
                    inputFrame = await ImageHelper.LoadVideoFrameFromStorageFileAsync(file);
                }
                else
                {
                    // Load image to VideoFrame
                    inputFrame = await ImageHelper.LoadVideoFrameFromFilePickedAsync();
                }
                if (inputFrame == null)
                {
                    NotifyUser("no valid image file selected", NotifyType.ErrorMessage);
                }
                else
                {
                    await Task.Run(() =>
                    {
                        EvaluateVideoFrame(inputFrame);
                    });
                }
            }
            catch (Exception ex)
            {
                Debug.WriteLine($"error: {ex.Message}");
                NotifyUser(ex.Message, NotifyType.ErrorMessage);
            }
            UIImageControls.IsEnabled = true;
            UIModelControls.IsEnabled = true;
        }

        /// <summary>
        /// 1) Bind input and output features 
        /// 2) Run evaluation of the model
        /// 3) Report the result
        /// </summary>
        /// <param name="inputVideoFrame"></param>
        /// <returns></returns>
        private void EvaluateVideoFrame(VideoFrame inputVideoFrame)
        {
            LearningModelSession session = null;
            bool showInitialImageAndProgress = true;

            {
                session = m_session;
                showInitialImageAndProgress = _showInitialImageAndProgress;
            }

            if ((inputVideoFrame != null) &&
                (inputVideoFrame.SoftwareBitmap != null || inputVideoFrame.Direct3DSurface != null) &&
                (session != null))
            {
                try
                {
                    //Dispatcher.RunAsync(CoreDispatcherPriority.Normal, () =>
                    //{
                    //    if (showInitialImageAndProgress)
                    //    {
                    //        UIProcessingProgressRing.IsActive = true;
                    //        UIProcessingProgressRing.Visibility = Visibility.Visible;
                    //        UIButtonSaveImage.IsEnabled = false;
                    //    }
                    //}).GetResults();


                    // for DX we need to create a new surface everytime
                    if (_useGPU)
                    {
                        // make it DX backed, on the same device that our inference is running on
                        var surface = _resultframeRenderer.BeginDraw((int)_outWidth, (int)_outHeight);
                        _outputFrame?.Dispose();
                        _outputFrame = VideoFrame.CreateWithDirect3D11Surface(surface);
                    }

                    // Bind and Eval
                    if (inputVideoFrame != null)
                    {
                        try
                        {
                            _perfStopwatch.Restart();

                            LearningModelBinding binding = new LearningModelBinding(session);

                            binding.Bind(m_outName, _outputFrame);
                            //TensorFloat tf = TensorFloat.Create();
                            //binding.Bind(m_outName, tf);
                            binding.Bind(m_inName, inputVideoFrame);

                            Int64 bindTime = _perfStopwatch.ElapsedMilliseconds;
                            Debug.WriteLine($"Binding: {bindTime}ms");

                            // render the input frame 
                            if (showInitialImageAndProgress)
                            {
                                ImageHelper.RenderFrameAsync(_inputFrameRenderer, inputVideoFrame).GetResults();
                            }

                            // Process the frame with the model
                            _perfStopwatch.Restart();

                            var results = m_session.Evaluate(binding, "test");

                            _perfStopwatch.Stop();
                            Int64 evalTime = _perfStopwatch.ElapsedMilliseconds;
                            Debug.WriteLine($"Eval: {evalTime}ms");

                            // Parse result
                            IReadOnlyDictionary<string, object> outputs = results.Outputs;
                            foreach (var output in outputs)
                            {
                                Debug.WriteLine($"{output.Key} : {output.Value} -> {output.Value.GetType()}");
                            }

                            // Display result, dont' wait for it to finish
                            ImageHelper.RenderFrameAsync(_resultframeRenderer, _outputFrame);
                        }
                        catch (Exception ex)
                        {
                            NotifyUser(ex.Message, NotifyType.ErrorMessage);
                            Debug.WriteLine(ex.ToString());
                        }

                        if (showInitialImageAndProgress)
                        {
                            //Dispatcher.RunAsync(CoreDispatcherPriority.Normal, () =>
                            //{
                            //    UIProcessingProgressRing.IsActive = false;
                            //    UIProcessingProgressRing.Visibility = Visibility.Collapsed;
                            //    UIButtonSaveImage.IsEnabled = true;
                            //}).GetResults();
                        }

                    }
                    else
                    {
                        Debug.WriteLine("Skipped eval, null input frame");
                    }
                }
                catch (Exception ex)
                {
                    NotifyUser(ex.Message, NotifyType.ErrorMessage);
                    Debug.WriteLine(ex.ToString());
                }

                _FramesPerSecond += 1;
                _perfStopwatch.Reset();
            }
        }

        /// <summary>
        /// Toggle inference device (GPU or CPU) from UI
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void UIToggleInferenceDevice_Toggled(object sender, RoutedEventArgs e)
        {
            Debug.WriteLine("UIToggleInferenceDevice_Toggled");
            if (UIStyleList == null)
            {
                return;
            }
            _useGPU = (bool)UIToggleInferenceDevice.IsOn;
            UIStyleList_SelectionChanged(null, null);
        }

        /// <summary>
        /// Change style to apply to the input frame
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void UIStyleList_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            Debug.WriteLine("UIStyleList_SelectionChanged");
            if (UIStyleList.SelectedIndex < 0)
            {
                return;
            }
            string selection = UIStyleList.SelectedValue.ToString();
            UIModelControls.IsEnabled = false;
            UIImageControls.IsEnabled = false;
            UIToggleInferenceDevice.IsEnabled = false;
            Task.Run(async () =>
            {
                _frameAquisitionLock.Wait();
                {
                    await LoadModelAsync(selection);
                }
                _frameAquisitionLock.Release();

            }).ContinueWith(async (antecedent) =>
        {
            if (antecedent.IsCompletedSuccessfully)
            {
                await Dispatcher.RunAsync(CoreDispatcherPriority.Normal, () =>
                {
                    // Create output frame
                    _outputFrame?.Dispose();
                    if (_useGPU)
                    {
                        // for DX we need to create the videoframe later, for each frame
                        _resultframeRenderer = new FrameRenderer(UIResultImage, m_session.Device.Direct3D11Device, (int)_outWidth, (int)_outHeight);
                        _inputFrameRenderer = new FrameRenderer(UIInputImage, m_session.Device.Direct3D11Device, (int)_outWidth, (int)_outHeight);
                    }
                    else
                    {
                        // make it CPU backed
                        _outputFrame = new VideoFrame(BitmapPixelFormat.Bgra8, (int)_outWidth, (int)_outHeight);
                        _resultframeRenderer = new FrameRenderer(UIResultImage);
                        _inputFrameRenderer = new FrameRenderer(UIInputImage);
                    }

                    NotifyUser($"Ready to stylize! ", NotifyType.StatusMessage);
                    UIImageControls.IsEnabled = true;
                    UIModelControls.IsEnabled = true;
                    UIToggleInferenceDevice.IsEnabled = true;
                    if (_isrocessingImages)
                    {
                        UIButtonFilePick_Click(null, null);
                    }
                });
            }
        });
        }

        /// <summary>
        /// Save image result to file
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private async void UIButtonSaveImage_Click(object sender, RoutedEventArgs e)
        {
            Debug.WriteLine("UIButtonSaveImage_Click");
            await ImageHelper.SaveVideoFrameToFilePickedAsync(_outputFrame);
        }

        private async void _inkEvaluationDispatcherTimer_Tick(object sender, object e)
        {
            if (!_frameAquisitionLock.Wait(100))
            {
                return;
            }
            {
                if (_isProcessingFrames)
                {
                    _frameAquisitionLock.Release();
                    return;
                }

                _isProcessingFrames = true;
            }
            _frameAquisitionLock.Release();

            try
            {
                if (UIInkControls.Visibility != Visibility.Visible)
                {
                    throw (new Exception("invisible control, will not attempt rendering"));
                }
                // Render the ink control to an image
                RenderTargetBitmap renderBitmap = new RenderTargetBitmap();
                await renderBitmap.RenderAsync(UIInkGrid);
                var buffer = await renderBitmap.GetPixelsAsync();
                var softwareBitmap = SoftwareBitmap.CreateCopyFromBuffer(buffer, BitmapPixelFormat.Bgra8, renderBitmap.PixelWidth, renderBitmap.PixelHeight, BitmapAlphaMode.Ignore);
                buffer = null;
                renderBitmap = null;

                // Instantiate VideoFrame using the softwareBitmap of the ink
                VideoFrame vf = VideoFrame.CreateWithSoftwareBitmap(softwareBitmap);

                // queue up the next frame for EvaluateThreadProc
                _frameAquisitionLock.Wait();
                {
                    _nextFrame = vf;
                }
                _frameAquisitionLock.Release();
            }
            catch (Exception ex)
            {
                Debug.WriteLine(ex.Message);
            }

            _frameAquisitionLock.Wait();
            {
                _isProcessingFrames = false;
            }
            _frameAquisitionLock.Release();

        }

        /// <summary>
        /// Apply effect on ink handdrawn
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private async void UIButtonInking_Click(object sender, RoutedEventArgs e)
        {
            Debug.WriteLine("UIButtonInking_Click");
            {
                await CleanupCameraAsync();
                CleanupInk();
                CleanupInputImage();
            }

            UIInkControls.Visibility = Visibility.Visible;
            UIResultImage.Width = UIInkControls.Width;
            UIResultImage.Height = UIInkControls.Height;
            _showInitialImageAndProgress = false;

            UIImageControls.IsEnabled = false;
            UIModelControls.IsEnabled = false;

            _inkEvaluationDispatcherTimer = new DispatcherTimer();
            _inkEvaluationDispatcherTimer.Tick += _inkEvaluationDispatcherTimer_Tick;
            _inkEvaluationDispatcherTimer.Interval = new TimeSpan(0, 0, 0, 0, 1000/30);
            _inkEvaluationDispatcherTimer.Start();

            UIImageControls.IsEnabled = true;
            UIModelControls.IsEnabled = true;
        }

        /// <summary>
        /// Cleanup inking resources
        /// </summary>
        private void CleanupInk()
        {
            Debug.WriteLine("CleanupInk");
            _frameAquisitionLock.Wait();
            try
            {
                if (_inkEvaluationDispatcherTimer != null)
                {
                    _inkEvaluationDispatcherTimer.Stop();
                    _inkEvaluationDispatcherTimer = null;
                }
                UIInkCanvasInput.InkPresenter.StrokeContainer.Clear();
                UIInkControls.Visibility = Visibility.Collapsed;
                _showInitialImageAndProgress = true;
                _isProcessingFrames = false;
                _nextFrame = null;
            }
            catch (Exception ex)
            {
                Debug.WriteLine($"CleanupInk: {ex.Message}");
            }
            finally
            {
                _frameAquisitionLock.Release();
            }
        }

        /// <summary>
        /// Cleanup input image resources
        /// </summary>
        private void CleanupInputImage()
        {
            Debug.WriteLine("CleanupInputImage");
            _frameAquisitionLock.Wait();
            try
            {
                UIInputImage.Visibility = Visibility.Collapsed;
                _isrocessingImages = false;
            }
            finally
            {
                _frameAquisitionLock.Release();
            }
        }

        /// <summary>
        /// Apply effect in real time to a camera feed
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private async void UIButtonLiveStream_Click(object sender, RoutedEventArgs e)
        {
            Debug.WriteLine("UIButtonLiveStream_Click");
            {
                await CleanupCameraAsync();
                CleanupInk();
                CleanupInputImage();
            }

            await InitializeMediaCaptureAsync();
        }

        /// <summary>
        /// Initialize MediaCapture for live stream scenario
        /// </summary>
        /// <returns></returns>
        private async Task InitializeMediaCaptureAsync()
        {
            Debug.WriteLine("InitializeMediaCaptureAsync");
            _frameAquisitionLock.Wait();
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
                NotifyUser(ex.Message, NotifyType.ErrorMessage);
                _mediaFrameSourceGroupList = null;
            }
            finally
            {
                _frameAquisitionLock.Release();
            }

            if ((_mediaFrameSourceGroupList == null) || (_mediaFrameSourceGroupList.Count == 0))
            {
                // No camera sources found
                Debug.WriteLine("No Camera found");
                NotifyUser("No Camera found", NotifyType.ErrorMessage);
                return;
            }

            var cameraNamesList = _mediaFrameSourceGroupList.Select(group => group.DisplayName);
            UICmbCamera.ItemsSource = cameraNamesList;
            UICmbCamera.SelectedIndex = 0;
        }

        /// <summary>
        /// Start previewing from the camera
        /// </summary>
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
            UIMediaPlayerElement.SetMediaPlayer(_mediaPlayer);
            UITxtBlockPreviewProperties.Text = string.Format("{0}x{1}@{2}, {3}",
                        _selectedMediaFrameSource.CurrentFormat.VideoFormat.Width,
                        _selectedMediaFrameSource.CurrentFormat.VideoFormat.Height,
                        _selectedMediaFrameSource.CurrentFormat.FrameRate.Numerator + "/" + _selectedMediaFrameSource.CurrentFormat.FrameRate.Denominator,
                        _selectedMediaFrameSource.CurrentFormat.Subtype);

            UICameraSelectionControls.Visibility = Visibility.Visible;
            UIMediaPlayerElement.Visibility = Visibility.Visible;
            UIResultImage.Width = UIMediaPlayerElement.Width;
            UIResultImage.Height = UIMediaPlayerElement.Height;
        }


        private static void EvaluateThreadProc(object param)
        {
            MainPage _this = (MainPage)param;
            while (true)
            {
                object frame;

                _this._frameAquisitionLock.Wait();
                {
                    frame = _this._nextFrame;
                    _this._nextFrame = null;
                }
                _this._frameAquisitionLock.Release();

                if (frame != null)
                {
                    VideoFrame vf = null;
                    if (frame is MediaFrameReference)
                    {
                        vf = ((MediaFrameReference)frame).VideoMediaFrame.GetVideoFrame();
                    }
                    else if (frame is VideoFrame)
                    {
                        vf = (VideoFrame)frame;
                    }

                    if (vf != null)
                    {
                        _this.EvaluateVideoFrame(vf);
                    }

                    if (frame is MediaFrameReference)
                    {
                        ((MediaFrameReference)frame).Dispose();
                    }
                }
                // Yield the rest of the time slice.
                Thread.Sleep(0);
            }
        }
        /// <summary>
        /// A new frame from the camera is available
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="args"></param>
        private async void _modelInputFrameReader_FrameArrived(MediaFrameReader sender, MediaFrameArrivedEventArgs args)
        {
            // queue up the next frame for EvaluateThreadProc
            _frameAquisitionLock.Wait();
            {
                var latestFrame = sender.TryAcquireLatestFrame();
                if (latestFrame != null)
                {
                    _nextFrame = latestFrame;
                }
            }
            _frameAquisitionLock.Release();
        }

        /// <summary>
        /// On selected camera changed
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private async void UICmbCamera_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            Debug.WriteLine("UICmbCamera_SelectionChanged");
            if ((_mediaFrameSourceGroupList.Count == 0) || (UICmbCamera.SelectedIndex < 0))
            {
                return;
            }

            {
                await CleanupCameraAsync();
                CleanupInk();
                CleanupInputImage();
            }

            UIImageControls.IsEnabled = false;
            UIModelControls.IsEnabled = false;

            _frameAquisitionLock.Wait();
            try
            {
                _selectedMediaFrameSourceGroup = _mediaFrameSourceGroupList[UICmbCamera.SelectedIndex];

                // Create MediaCapture and its settings
                _mediaCapture = new MediaCapture();
                var settings = new MediaCaptureInitializationSettings
                {
                    SourceGroup = _selectedMediaFrameSourceGroup,
                    PhotoCaptureSource = PhotoCaptureSource.Auto,
                    //MemoryPreference = MediaCaptureMemoryPreference.Cpu,
                    MemoryPreference = MediaCaptureMemoryPreference.Auto,
                    StreamingCaptureMode = StreamingCaptureMode.Video
                };

                // Initialize MediaCapture
                await _mediaCapture.InitializeAsync(settings);
                StartPreview();

                if (m_model != null)
                {
                    await InitializeModelInputFrameReaderAsync();
                }
            }
            catch (Exception ex)
            {
                NotifyUser(ex.Message, NotifyType.ErrorMessage);
                Debug.WriteLine(ex.ToString());
            }
            finally
            {
                _frameAquisitionLock.Release();
            }

            UIImageControls.IsEnabled = true;
            UIModelControls.IsEnabled = true;
        }

        /// <summary>
        /// Initialize the camera frame reader and preview UI element
        /// </summary>
        /// <returns></returns>
        private async Task InitializeModelInputFrameReaderAsync()
        {
            Debug.WriteLine("InitializeModelInputFrameReaderAsync");
            // Create the MediaFrameReader
            try
            {
                if (_modelInputFrameReader != null)
                {
                    await _modelInputFrameReader.StopAsync();
                    _modelInputFrameReader.FrameArrived -= _modelInputFrameReader_FrameArrived;
                }

                string frameReaderSubtype = _selectedMediaFrameSource.CurrentFormat.Subtype;
                if (string.Compare(frameReaderSubtype, MediaEncodingSubtypes.Nv12, true) != 0 &&
                    string.Compare(frameReaderSubtype, MediaEncodingSubtypes.Bgra8, true) != 0 &&
                    string.Compare(frameReaderSubtype, MediaEncodingSubtypes.Yuy2, true) != 0 &&
                    string.Compare(frameReaderSubtype, MediaEncodingSubtypes.Rgb32, true) != 0)
                {
                    frameReaderSubtype = MediaEncodingSubtypes.Bgra8;
                }

                _modelInputFrameReader = null;
                _modelInputFrameReader = await _mediaCapture.CreateFrameReaderAsync(_selectedMediaFrameSource, frameReaderSubtype);
                _modelInputFrameReader.AcquisitionMode = MediaFrameReaderAcquisitionMode.Realtime;

                await _modelInputFrameReader.StartAsync();

                _modelInputFrameReader.FrameArrived += _modelInputFrameReader_FrameArrived;
                _showInitialImageAndProgress = false;
            }
            catch (Exception ex)
            {
                NotifyUser($"Error while initializing MediaframeReader: " + ex.Message, NotifyType.ErrorMessage);
                Debug.WriteLine(ex.ToString());
            }
        }

        /// <summary>
        /// Cleanup camera used for live stream scenario
        /// </summary>
        private async Task CleanupCameraAsync()
        {
            Debug.WriteLine("CleanupCameraAsync");
            _frameAquisitionLock.Wait();
            try
            {
                _nextFrame = null;
                _showInitialImageAndProgress = true;
                if (_modelInputFrameReader != null)
                {
                    _modelInputFrameReader.FrameArrived -= _modelInputFrameReader_FrameArrived;
                }
                _modelInputFrameReader = null;

                if (_mediaCapture != null)
                {
                    _mediaCapture = null;
                }
                if (_mediaPlayer != null)
                {
                    _mediaPlayer.Source = null;
                    _mediaPlayer = null;
                }
            }
            catch (Exception ex)
            {
                Debug.WriteLine($"CleanupCameraAsync: {ex.Message}");
            }
            finally
            {
                _frameAquisitionLock.Release();
            }

            await Dispatcher.RunAsync(CoreDispatcherPriority.Normal, () =>
            {
                UICameraSelectionControls.Visibility = Visibility.Collapsed;
                UIMediaPlayerElement.Visibility = Visibility.Collapsed;
            });
        }
    }
    public enum NotifyType
    {
        StatusMessage,
        ErrorMessage
    };

    public sealed class FrameRenderer
    {
        private Image _imageElement;
        private SoftwareBitmap _backBuffer;
        private bool _taskRunning = false;
        private DXRenderComponent.RenderHelper _renderHelper;

        // use a DX renderer
        public FrameRenderer(Image imageElement, IDirect3DDevice device, int width, int heigth)
        {
            _imageElement = imageElement;
            _imageElement.Source = new SurfaceImageSource(width, heigth);
            _renderHelper = new DXRenderComponent.RenderHelper(_imageElement.Source as SurfaceImageSource, device);
        }
        // use a CPU renderer
        public FrameRenderer(Image imageElement)
        {
            _imageElement = imageElement;
            _imageElement.Source = new SoftwareBitmapSource();
        }

        public void EndDraw()
        {
            // EndDraw will actually trigger rendering, this need to happen on the render thread
            var task = _imageElement.Dispatcher.RunAsync(CoreDispatcherPriority.Normal,
                async () =>
                {
                    _renderHelper.EndDraw();
                });

        }

        public IDirect3DSurface BeginDraw(int width, int height)
        {
            return _renderHelper.BeginDraw(width, height);
        }

        public void RenderFrame(SoftwareBitmap softwareBitmap)
        {
            if (softwareBitmap != null)
            {
                // Swap the processed frame to _backBuffer and trigger UI thread to render it
                _backBuffer = softwareBitmap;

                // Changes to xaml ImageElement must happen in UI thread through Dispatcher
                var task = _imageElement.Dispatcher.RunAsync(CoreDispatcherPriority.Normal,
                    async () =>
                    {
                        // Don't let two copies of this task run at the same time.
                        if (_taskRunning)
                        {
                            return;
                        }
                        _taskRunning = true;
                        try
                        {
                            var imageSource = (SoftwareBitmapSource)_imageElement.Source;
                            await imageSource.SetBitmapAsync(_backBuffer);
                        }
                        catch (Exception ex)
                        {
                            Debug.WriteLine(ex.Message);
                        }
                        _taskRunning = false;
                    });
            }
            else
            {
                var task = _imageElement.Dispatcher.RunAsync(CoreDispatcherPriority.Normal,
                    async () =>
                    {
                        var imageSource = (SoftwareBitmapSource)_imageElement.Source;
                        await imageSource.SetBitmapAsync(null);
                    });
            }
        }
    }
}
