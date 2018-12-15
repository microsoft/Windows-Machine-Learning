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
        private string _modelFileName = null;
        private LearningModelSession _session;
        int _outWidth, _outHeight, _inWidth, _inHeight;
        string m_outName, m_inName;
        private List<string> _labels = new List<string>();
        object _nextFrame = null;  // can be a MediaFrameReference or a VideoFrame

        VideoFrame _outputFrame = null;
        LearningModelBinding _binding = null;

        class FrameResources
        {
            ~FrameResources()
            {
                _outputFrame?.Dispose();
            }
            public VideoFrame _outputFrame;
            public LearningModelBinding _binding;
        }
        FrameResources[] _frameResources = null;

        // Debug
        private Stopwatch _perfStopwatch = new Stopwatch(); // performance Stopwatch used throughout
        private DispatcherTimer _FramesPerSecondTimer = new DispatcherTimer();
        private long _CaptureFPS = 0;
        private long _RenderFPS = 0;

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
            // how many frames did we capture?
            long intervalFPS = _CaptureFPS;
            _CaptureFPS = 0;
            NotifyUser(CaptureFPS, $"{intervalFPS}", NotifyType.StatusMessage);

            // how many frames did we render?
            intervalFPS = _RenderFPS;
            _RenderFPS = 0;
            NotifyUser(RenderFPS, $"{intervalFPS}", NotifyType.StatusMessage);
        }

        /// <summary>
        /// Display a message to the user.
        /// This method may be called from any thread.
        /// </summary>
        /// <param name="strMessage"></param>
        /// <param name="type"></param>
        public void NotifyUser(TextBlock block, string strMessage, NotifyType type)
        {
            // If called from the UI thread, then update immediately.
            // Otherwise, schedule a task on the UI thread to perform the update.
            if (Dispatcher.HasThreadAccess)
            {
                UpdateStatus(block, strMessage, type);
            }
            else
            {
                var task = Dispatcher.RunAsync(CoreDispatcherPriority.Normal, () => UpdateStatus(block, strMessage, type));
                task.AsTask().Wait();
            }
        }

        /// <summary>
        /// Update the status message displayed on the UI
        /// </summary>
        /// <param name="strMessage"></param>
        /// <param name="type"></param>
        private void UpdateStatus(TextBlock block, string strMessage, NotifyType type)
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

            block.Text = strMessage;

            //// Collapse the StatusBlock if it has no text to conserve real estate.
            //UIStatusBorder.Visibility = (block.Text != String.Empty) ? Visibility.Visible : Visibility.Collapsed;
            //if (block.Text != String.Empty)
            //{
            //    UIStatusBorder.Visibility = Visibility.Visible;
            //    UIStatusPanel.Visibility = Visibility.Visible;
            //}
            //else
            //{
            //    UIStatusBorder.Visibility = Visibility.Collapsed;
            //    UIStatusPanel.Visibility = Visibility.Collapsed;
            //}
        }

        /// <summary>
        /// Load the labels and model and initialize WinML
        /// </summary>
        /// <returns></returns>
        private async Task LoadModelAsync(string modelFileName, IDirect3DDevice device = null)
        {
            _session?.Model.Dispose();
            _session?.Dispose();
            _session = null;
            try
            {
                // Start stopwatch
                _perfStopwatch.Restart();

                // Load Model
                StorageFile modelFile = await StorageFile.GetFileFromApplicationUriAsync(new Uri($"ms-appx:///Assets/{modelFileName}.onnx"));
                LearningModel model = await LearningModel.LoadFromStorageFileAsync(modelFile);

                // Stop stopwatch
                _perfStopwatch.Stop();

                // Setting preferred inference device given user's intent
                if (_useGPU && device != null)
                {
                    _session = new LearningModelSession(model, LearningModelDevice.CreateFromDirect3D11Device(device));
                }
                else if (_useGPU)
                {
                    _session = new LearningModelSession(model, new LearningModelDevice(LearningModelDeviceKind.DirectXHighPerformance));
                }
                else
                {
                    _session = new LearningModelSession(model, new LearningModelDevice(LearningModelDeviceKind.Cpu));
                }

                // Debugging logic to see the input and output of ther model and retrieve dimensions of input/output variables
                // ### DEBUG ###
                foreach (var inputF in model.InputFeatures)
                {
                    Debug.WriteLine($"input | kind:{inputF.Kind}, name:{inputF.Name}, type:{inputF.GetType()}");
                    int i = 0;
                    ImageFeatureDescriptor imgDesc = inputF as ImageFeatureDescriptor;
                    TensorFeatureDescriptor tfDesc = inputF as TensorFeatureDescriptor;
                    _inWidth = (int)(imgDesc == null ? tfDesc.Shape[3] : imgDesc.Width);
                    _inHeight = (int)(imgDesc == null ? tfDesc.Shape[2] : imgDesc.Height);
                    m_inName = inputF.Name;

                    Debug.WriteLine($"N: {(imgDesc == null ? tfDesc.Shape[0] : 1)}, " +
                        $"Channel: {(imgDesc == null ? tfDesc.Shape[1].ToString() : imgDesc.BitmapPixelFormat.ToString())}, " +
                        $"Height:{(imgDesc == null ? tfDesc.Shape[2] : imgDesc.Height)}, " +
                        $"Width: {(imgDesc == null ? tfDesc.Shape[3] : imgDesc.Width)}");
                }
                foreach (var outputF in model.OutputFeatures)
                {
                    Debug.WriteLine($"output | kind:{outputF.Kind}, name:{outputF.Name}, type:{outputF.GetType()}");
                    int i = 0;
                    ImageFeatureDescriptor imgDesc = outputF as ImageFeatureDescriptor;
                    TensorFeatureDescriptor tfDesc = outputF as TensorFeatureDescriptor;
                    _outWidth = (int)(imgDesc == null ? tfDesc.Shape[3] : imgDesc.Width);
                    _outHeight = (int)(imgDesc == null ? tfDesc.Shape[2] : imgDesc.Height);
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
                NotifyUser(StatusBlock, $"error: {ex.Message}", NotifyType.ErrorMessage);
                Debug.WriteLine($"error: {ex.Message}");
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
                    EvaluateVideoFrame(vf, null);
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
                    NotifyUser(StatusBlock, "no valid image file selected", NotifyType.ErrorMessage);
                }
                else
                {
                    await Task.Run(() =>
                    {
                        EvaluateVideoFrame(inputFrame, null);
                    });
                }
            }
            catch (Exception ex)
            {
                Debug.WriteLine($"error: {ex.Message}");
                NotifyUser(StatusBlock, ex.Message, NotifyType.ErrorMessage);
            }
            UIImageControls.IsEnabled = true;
            UIModelControls.IsEnabled = true;
        }

        private void RenderFrame(FrameRenderer renderer, VideoFrame frame, bool outputFrame = false)
        {
            // run this on the UI thread
            var task = Dispatcher.RunAsync(CoreDispatcherPriority.Normal,
                async () =>
                {
                    bool useDX = (frame.SoftwareBitmap == null);
                    if (useDX)
                    {
                        Debug.WriteLine($"Presenting BackBuffer{_resultframeRenderer.GetBackBufferIndex()}");
                        renderer.Present();
                    }
                    else
                    {
                        renderer.RenderFrame(frame.SoftwareBitmap);
                    }
                    _RenderFPS += 1;
                    if (outputFrame)
                    {
                        if (_useGPU)
                        {
                            var fr = _frameResources[_resultframeRenderer.GetBackBufferIndex()];
                            if (fr == null)
                            {
                                // make our output frames and binding objects          
                                fr = new FrameResources();
                                var surface = _resultframeRenderer.GetBackBuffer();
                                fr._outputFrame = VideoFrame.CreateWithDirect3D11Surface(surface);
                                fr._binding = new LearningModelBinding(_session);
                                _frameResources[_resultframeRenderer.GetBackBufferIndex()] = fr;
                            }

                            Debug.WriteLine($"Using BackBuffer{_resultframeRenderer.GetBackBufferIndex()}");

                            _outputFrame = fr._outputFrame;
                            _binding = fr._binding;
                        }
                        else
                        {
                            // CPU
                            _outputFrame = new VideoFrame(BitmapPixelFormat.Bgra8, _outWidth, _outHeight);
                        }
                    }
                });
        }

        /// <summary>
        /// 1) Bind input and output features 
        /// 2) Run evaluation of the model
        /// 3) Report the result
        /// </summary>
        /// <param name="inputVideoFrame"></param>
        /// <returns></returns>
        private void EvaluateVideoFrame(VideoFrame inputVideoFrame, VideoFrame outputFrame)
        {
            LearningModelSession session = null;
            bool showInitialImageAndProgress = true;

            {
                session = _session;
                showInitialImageAndProgress = _showInitialImageAndProgress;
            }

            if ((inputVideoFrame != null) &&
                (inputVideoFrame.SoftwareBitmap != null || inputVideoFrame.Direct3DSurface != null) &&
                (session != null) &&
                outputFrame != null)
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


                    // Bind and Eval
                    if (true)
                    {
                        _perfStopwatch.Restart();
                        BitmapBounds inBounds = new BitmapBounds();
                        BitmapBounds outBounds = new BitmapBounds();

                        Rect r = ImageHelper.GetCropRect(
                            inputVideoFrame.Direct3DSurface.Description.Width,
                            inputVideoFrame.Direct3DSurface.Description.Height,
                            (uint)_outWidth,
                            (uint)_outHeight);
                        inBounds.X = (uint)r.Left;
                        inBounds.Y = (uint)r.Top;
                        inBounds.Width = (uint)r.Right;
                        inBounds.Height = (uint)r.Bottom;
                        outBounds.X = outBounds.Y = 0;
                        outBounds.Width = (uint)_outWidth;
                        outBounds.Height = (uint)_outHeight;
                        inputVideoFrame.CopyToAsync(outputFrame, inBounds, outBounds).GetResults();
                        RenderFrame(_resultframeRenderer, outputFrame, true);
                    }
                    else if (inputVideoFrame != null)
                    {
                        try
                        {
                            _perfStopwatch.Restart();

                            // clear out the results from the previous binding
                            _binding?.Clear();
                            _binding.Bind(m_outName, outputFrame);
                            _binding.Bind(m_inName, inputVideoFrame);

                            Int64 bindTime = _perfStopwatch.ElapsedMilliseconds;
                            Debug.WriteLine($"Binding: {bindTime}ms");

                            // render the input frame 
                            if (showInitialImageAndProgress)
                            {
                                RenderFrame(_inputFrameRenderer, inputVideoFrame);
                            }

                            // Process the frame with the model
                            _perfStopwatch.Restart();

                            var results = _session.Evaluate(_binding, "test");

                            _perfStopwatch.Stop();
                            Int64 evalTime = _perfStopwatch.ElapsedMilliseconds;
                            Debug.WriteLine($"Eval: {evalTime}ms");

                            // Display result, dont' wait for it to finish
                            RenderFrame(_resultframeRenderer, outputFrame, true);
                        }
                        catch (Exception ex)
                        {
                            NotifyUser(StatusBlock, ex.Message, NotifyType.ErrorMessage);
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
                    NotifyUser(StatusBlock, ex.Message, NotifyType.ErrorMessage);
                    Debug.WriteLine(ex.ToString());
                }

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

        private void WaitForFrameRendering()
        {
            if (_resultframeRenderer != null)
            {
                _resultframeRenderer.WaitForFrameRendering();
            }
        }

        private void InitializeFrameRendering()
        {
            _outputFrame?.Dispose();

            // create the frame renderers and output frames
            if (_useGPU)
            {
                // switch from the image xaml control to the swapchain xaml control
                UIResultImage.Visibility = Visibility.Collapsed;
                UIResultSwapChainPanel.Visibility = Visibility.Visible;

                _resultframeRenderer = new FrameRenderer(UIResultSwapChainPanel, _session.Device.Direct3D11Device, _outWidth, _outHeight);
                //_inputFrameRenderer = new FrameRenderer(UIInputImage, _session.Device.Direct3D11Device, _outWidth, _outHeight);

                // make our output frames and binding objects          
                _frameResources = new FrameResources[_resultframeRenderer.GetBufferCount()];
                var fr = new FrameResources();
                var surface = _resultframeRenderer.GetBackBuffer();
                fr._outputFrame = VideoFrame.CreateWithDirect3D11Surface(surface);
                fr._binding = new LearningModelBinding(_session);
                _frameResources[_resultframeRenderer.GetBackBufferIndex()] = fr;

                _outputFrame = fr._outputFrame;
                _binding = fr._binding;
            }
            else
            {
                // CPU
                _resultframeRenderer = new FrameRenderer(UIResultImage);
                _inputFrameRenderer = new FrameRenderer(UIInputImage);
                _outputFrame = new VideoFrame(BitmapPixelFormat.Bgra8, (int) _outWidth, (int) _outHeight);
            }
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

            // save the model filename for when we load the model
            _modelFileName = selection;

            NotifyUser(StatusBlock, $"Ready to stylize! ", NotifyType.StatusMessage);

            UIImageControls.IsEnabled = true;
            UIModelControls.IsEnabled = true;
            UIToggleInferenceDevice.IsEnabled = true;

            // show an initial image
            if (_isrocessingImages)
            {
                //UIButtonFilePick_Click(null, null);
            }
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

                _frameAquisitionLock.Wait();
                {
                    // make sure the queued frame for EvaluateThreadProc is always the latest frame 
                    _nextFrame = vf;
                    _CaptureFPS += 1;
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
                NotifyUser(StatusBlock, ex.Message, NotifyType.ErrorMessage);
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
                NotifyUser(StatusBlock, "No Camera found", NotifyType.ErrorMessage);
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
                VideoFrame outputFrame = null;

                _this.WaitForFrameRendering();

                _this._frameAquisitionLock.Wait();
                {
                    frame = _this._nextFrame;
                    _this._nextFrame = null;
                    if (frame != null)
                    {
                        outputFrame = _this._outputFrame;
                        _this._outputFrame = null;
                    }
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
                        _this.EvaluateVideoFrame(vf, outputFrame);
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
            _frameAquisitionLock.Wait();
            {
                var latestFrame = sender.TryAcquireLatestFrame();
                if (latestFrame != null)
                {
                    // do we have an output frame available?
                    if (_outputFrame != null)
                    {
                        // make sure the queued frame for EvaluateThreadProc is always the latest frame 
                        _nextFrame = latestFrame;
                    }
                }
                _CaptureFPS += 1;
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

                // initialize the MediaCapture
                await _mediaCapture.InitializeAsync(settings);

                // start up the camera preview
                StartPreview();

                // (re)load the model, if the camera just changed we might want to run the model on the 
                // capture device
                await LoadModelAsync(_modelFileName, _mediaCapture.MediaCaptureSettings.Direct3D11Device);

                // Reset the frame renderers
                InitializeFrameRendering();

                // initialize the frame reader
                await InitializeModelInputFrameReaderAsync();
            }
            catch (Exception ex)
            {
                NotifyUser(StatusBlock, ex.Message, NotifyType.ErrorMessage);
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
                NotifyUser(StatusBlock, $"Error while initializing MediaframeReader: " + ex.Message, NotifyType.ErrorMessage);
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
        private DXRenderComponent.ImageSourceRenderHelper _imageHelper;
        private DXRenderComponent.SwapChainPanelRenderHelper _panelHelper;

        public FrameRenderer(SwapChainPanel panel, IDirect3DDevice device, int width, int heigth)
        {
            _panelHelper = new DXRenderComponent.SwapChainPanelRenderHelper(panel, device, (uint)width, (uint)heigth);
        }

        // use a DX renderer
        public FrameRenderer(Image imageElement, IDirect3DDevice device, int width, int heigth)
        {
            _imageElement = imageElement;
            var surfaceSource = new SurfaceImageSource(width, heigth);
            _imageElement.Source = surfaceSource;
            _imageHelper = new DXRenderComponent.ImageSourceRenderHelper(surfaceSource, device);
        }
        // use a CPU renderer
        public FrameRenderer(Image imageElement)
        {
            _imageElement = imageElement;
            _imageElement.Source = new SoftwareBitmapSource();
        }

        public void WaitForFrameRendering()
        {
            if (_panelHelper != null)
            {
                _panelHelper.WaitForPresentReady();
            }
        }

        public UInt32 GetBackBufferIndex()
        {
            return _panelHelper.GetBackBufferIndex();
        }

        public UInt32 GetBufferCount()
        {
            return _panelHelper.GetBufferCount();
        }

        public IDirect3DSurface GetBackBuffer()
        {
            return _panelHelper.GetBackBuffer();
        }

        public void Present()
        {
            _panelHelper.Present();
        }

        public void EndDraw()
        {
            _imageHelper.EndDraw();
        }

        public IDirect3DSurface BeginDraw(int width, int height)
        {
            UInt32 w = (UInt32)width;
            UInt32 h = (UInt32)height;
            return _imageHelper.BeginDraw(w, h);
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
