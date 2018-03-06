using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Threading.Tasks;
using Windows.Media.Capture;
using Windows.Storage;
using Windows.UI.Core;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Navigation;
using Windows.Graphics.Imaging;
using Windows.AI.MachineLearning.Preview;
using Windows.Media;
using System.Threading;

namespace StyleTransfer
{
    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class MainPage : Page
    {
        // States
        private bool _isReadyForEval = true;
        private SemaphoreSlim _evaluationLock = new SemaphoreSlim(1);
        private bool _useGPU = true;

        // Rendering related
        private FrameRenderer _resultframeRenderer;
        private FrameRenderer _inputFrameRenderer;

        // WinML related
        private const string _kModelFileName = "Candy";
        private const string _kDefaultImageFileName = "DefaultImage.jpg";
        private ImageVariableDescriptorPreview _inputImageDescription;
        private ImageVariableDescriptorPreview _outputImageDescription;
        private LearningModelPreview _model;
        private LearningModelBindingPreview _binding = null;
        VideoFrame _inputFrame = null;
        VideoFrame _outputFrame = null;

        public MainPage()
        {
            this.InitializeComponent();
        }

        protected override void OnNavigatedTo(NavigationEventArgs e)
        {
            _resultframeRenderer = new FrameRenderer(UIResultImage);
            _inputFrameRenderer = new FrameRenderer(UIInputImage);
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
        private async Task LoadModelAsync()
        {
            _evaluationLock.Wait();
            {
                _binding = null;
                _model = null;
                _isReadyForEval = false;

                try
                {
                    // Load Model
                    StorageFile modelFile = await StorageFile.GetFileFromApplicationUriAsync(new Uri($"ms-appx:///Assets/{_kModelFileName}.onnx"));
                    _model = await LearningModelPreview.LoadModelFromStorageFileAsync(modelFile);

                    // Hardcoding to use GPU
                    InferencingOptionsPreview options = _model.InferencingOptions;
                    options.PreferredDeviceKind = _useGPU ? LearningModelDeviceKindPreview.LearningDeviceGpu : LearningModelDeviceKindPreview.LearningDeviceCpu;
                    _model.InferencingOptions = options;

                    // Debugging logic to see the input and output of ther model
                    List<ILearningModelVariableDescriptorPreview> inputFeatures = _model.Description.InputFeatures.ToList();
                    List<ILearningModelVariableDescriptorPreview> outputFeatures = _model.Description.OutputFeatures.ToList();
                    var metadata = _model.Description.Metadata;
                    foreach (var md in metadata)
                    {
                        Debug.WriteLine($"{md.Key} | {md.Value}");
                    }

                    _inputImageDescription =
                        inputFeatures.FirstOrDefault(feature => feature.ModelFeatureKind == LearningModelFeatureKindPreview.Image)
                        as ImageVariableDescriptorPreview;

                    _outputImageDescription =
                        outputFeatures.FirstOrDefault(feature => feature.ModelFeatureKind == LearningModelFeatureKindPreview.Image)
                        as ImageVariableDescriptorPreview;

                    _isReadyForEval = true;
                }
                catch (Exception ex)
                {
                    NotifyUser($"error: {ex.Message}", NotifyType.ErrorMessage);
                    Debug.WriteLine($"error: {ex.Message}");
                }
            }
            _evaluationLock.Release();
        }

        /// <summary>
        /// Acquire manually an image from the camera preview stream
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private async void UIButtonAcquireImage_Click(object sender, RoutedEventArgs e)
        {
            UIInputImage.Visibility = Visibility.Visible;
            UIProcessingProgressRing.IsActive = true;
            UIProcessingProgressRing.Visibility = Visibility.Visible;
            UIButtonSaveImage.IsEnabled = false;

            CameraCaptureUI dialog = new CameraCaptureUI();
            dialog.PhotoSettings.AllowCropping = false;
            dialog.PhotoSettings.Format = CameraCaptureUIPhotoFormat.Png;

            StorageFile file = await dialog.CaptureFileAsync(CameraCaptureUIMode.Photo);
            if (file != null)
            {
                var vf = await ImageHelper.LoadVideoFrameFromStorageFileAsync(file);
                await Task.Run(async () =>
                {
                    await EvaluateVideoFrameAsync(vf);
                });
            }
        }

        /// <summary>
        /// Select and evaluate a picture
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private async void UIButtonFilePick_Click(object sender, RoutedEventArgs e)
        {
            UIInputImage.Visibility = Visibility.Visible;
            UIProcessingProgressRing.IsActive = true;
            UIProcessingProgressRing.Visibility = Visibility.Visible;
            UIButtonSaveImage.IsEnabled = false;

            try
            {
                VideoFrame inputFrame = null;

                // use a default image or a picture selected by the user
                if (sender == null && e == null)
                {
                    // use a default image..
                    if (_inputFrame == null)
                    {
                        var file = await StorageFile.GetFileFromApplicationUriAsync(new Uri($"ms-appx:///Assets/{_kDefaultImageFileName}"));
                        inputFrame = await ImageHelper.LoadVideoFrameFromStorageFileAsync(file);
                    }
                    // ..or use a picture already selected by the user..
                    else
                    {
                        inputFrame = _inputFrame;
                    }
                }
                // ..or use a new picture selected by the user
                else
                {
                    inputFrame = await ImageHelper.LoadVideoFrameFromFilePickedAsync();
                }

                // if the picture is valid, let's process it with the model
                if (inputFrame == null)
                {
                    NotifyUser("no valid image file selected", NotifyType.ErrorMessage);
                }
                else
                {
                    await Task.Run(async () => await EvaluateVideoFrameAsync(inputFrame));
                }
            }
            catch (Exception ex)
            {
                Debug.WriteLine($"error: {ex.Message}");
                NotifyUser(ex.Message, NotifyType.ErrorMessage);
            }
        }

        /// <summary>
        /// 1) Bind input and output features 
        /// 2) Run evaluation of the model
        /// 3) Retrieve the result
        /// </summary>
        /// <param name="inputVideoFrame"></param>
        /// <returns></returns>
        private async Task EvaluateVideoFrameAsync(VideoFrame inputVideoFrame)
        {
            LearningModelPreview model = null;
            bool isReadyForEval = false;
            _evaluationLock.Wait();
            {
                model = _model;
                isReadyForEval = _isReadyForEval;
                _isReadyForEval = false;
            }
            _evaluationLock.Release();

            if (inputVideoFrame != null && isReadyForEval && model != null)
            {
                _inputFrame = inputVideoFrame;
                try
                {
                    NotifyUser("Processing...", NotifyType.StatusMessage);

                    if (_outputFrame == null)
                    {
                        _outputFrame = new VideoFrame(BitmapPixelFormat.Bgra8, (int)_outputImageDescription.Width, (int)_outputImageDescription.Height);
                    }

                    // Create bindings for the input and output buffers
                    if (_binding == null)
                    {
                        _binding = new LearningModelBindingPreview(model as LearningModelPreview);

                        // since we reuse the output at each evaluation, bind it only once
                        _binding.Bind(_outputImageDescription.Name, _outputFrame);
                    }
                    _binding.Bind(_inputImageDescription.Name, inputVideoFrame);

                    // Render the input frame 
                    await ImageHelper.RenderFrameAsync(_inputFrameRenderer, inputVideoFrame);

                    // Process the frame with the model
                    var results = await _model.EvaluateAsync(_binding, "test");

                    // Parse result
                    IReadOnlyDictionary<string, object> outputs = results.Outputs;
                    foreach (var output in outputs)
                    {
                        Debug.WriteLine($"{output.Key} : {output.Value} -> {output.Value.GetType()}");
                    }

                    // Display result
                    VideoFrame vf = results.Outputs[_outputImageDescription.Name] as VideoFrame;
                    await ImageHelper.RenderFrameAsync(_resultframeRenderer, vf);

                    await Dispatcher.RunAsync(CoreDispatcherPriority.Normal, () =>
                    {
                        UIProcessingProgressRing.IsActive = false;
                        UIProcessingProgressRing.Visibility = Visibility.Collapsed;
                        UIButtonSaveImage.IsEnabled = true;
                        UIToggleInferenceDevice.IsEnabled = true;
                    });

                    NotifyUser("Done!", NotifyType.StatusMessage);
                }
                catch (Exception ex)
                {
                    NotifyUser(ex.Message, NotifyType.ErrorMessage);
                    Debug.WriteLine(ex.ToString());
                }

                _evaluationLock.Wait();
                {
                    _isReadyForEval = true;
                }
                _evaluationLock.Release();
            }
        }

        /// <summary>
        /// Toggle inference device (GPU or CPU) from UI
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void UIToggleInferenceDevice_Toggled(object sender, RoutedEventArgs e)
        {
            _useGPU = (bool)UIToggleInferenceDevice.IsOn;
            UIToggleInferenceDevice.IsEnabled = false;

            // Reload model
            Task.Run(async () => await LoadModelAsync()).ContinueWith(async (antecedent) =>
            {
                if (antecedent.IsCompletedSuccessfully && _isReadyForEval)
                {
                    await Dispatcher.RunAsync(CoreDispatcherPriority.Normal, () =>
                    {
                        NotifyUser($"Ready to stylize! ", NotifyType.StatusMessage);
                        UIImageControls.IsEnabled = true;
                        UIModelControls.IsEnabled = true;

                        UIButtonFilePick_Click(null, null);
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
            await ImageHelper.SaveVideoFrameToFilePickedAsync(_outputFrame);
        }
    }

    public enum NotifyType
    {
        StatusMessage,
        ErrorMessage
    };
}
