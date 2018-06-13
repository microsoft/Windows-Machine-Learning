using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.AI.MachineLearning.Preview;
using Windows.Storage;
using Windows.Media;
using Windows.Graphics.Imaging;
using System.Threading.Tasks;
using Windows.Storage.Streams;
using Windows.UI.Core;
using Windows.Storage.Pickers;
using Windows.UI.Xaml.Media.Imaging;
using System.Diagnostics;
using Newtonsoft.Json;

namespace SqueezeNetObjectDetection
{
    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class MainPage : Page
    {
        private const string _kModelFileName = "SqueezeNet.onnx";
        private const string _kLabelsFileName = "Labels.json";
        private ImageVariableDescriptorPreview _inputImageDescription;
        private TensorVariableDescriptorPreview _outputTensorDescription;
        private LearningModelPreview _model = null;
        private List<string> _labels = new List<string>();
        List<float> _outputVariableList = new List<float>();
        private Stopwatch modeltime_w = new Stopwatch();

        public MainPage()
        {
            this.InitializeComponent();
        }

        /// <summary>
        /// Load the label and model files
        /// </summary>
        /// <returns></returns>
        private async Task LoadModelAsync()
        {
            // just load the model one time.
            if (_model != null) return;

            StatusBlock.Text = $"Loading {_kModelFileName} ... patience ";

            try
            {
                // Parse labels from label json file.  We know the file's 
                // entries are already sorted in order.
                var fileString = File.ReadAllText($"Assets/{_kLabelsFileName}");
                var fileDict = JsonConvert.DeserializeObject<Dictionary<string,string>>(fileString);
                foreach( var kvp in fileDict)
                {
                    _labels.Add(kvp.Value);
                }

                // legacy code for comparison
/*
                var labels = new List<string>();
                var file = await StorageFile.GetFileFromApplicationUriAsync(new Uri($"ms-appx:///Assets/{_kLabelsFileName}"));
                using (var inputStream = await file.OpenReadAsync())
                using (var classicStream = inputStream.AsStreamForRead())
                using (var streamReader = new StreamReader(classicStream))
                {
                    string line = "";
                    char[] charToTrim = { '\"', ' ' };
                    while (streamReader.Peek() >= 0)
                    {
                        line = streamReader.ReadLine();
                        line.Trim(charToTrim);
                        var indexAndLabel = line.Split(':');
                        if (indexAndLabel.Count() == 2)
                        {
                            labels.Add(indexAndLabel[1]);
                        }
                    }
                }
*/
                // Load Model
                var modelFile = await StorageFile.GetFileFromApplicationUriAsync(new Uri($"ms-appx:///Assets/{_kModelFileName}"));
                _model = await LearningModelPreview.LoadModelFromStorageFileAsync(modelFile);

                // Retrieve model input and output variable descriptions (we already know the model takes an image in and outputs a tensor)
                List<ILearningModelVariableDescriptorPreview> inputFeatures = _model.Description.InputFeatures.ToList();
                List<ILearningModelVariableDescriptorPreview> outputFeatures = _model.Description.OutputFeatures.ToList();

                _inputImageDescription =
                    inputFeatures.FirstOrDefault(feature => feature.ModelFeatureKind == LearningModelFeatureKindPreview.Image)
                    as ImageVariableDescriptorPreview;

                _outputTensorDescription =
                    outputFeatures.FirstOrDefault(feature => feature.ModelFeatureKind == LearningModelFeatureKindPreview.Tensor)
                    as TensorVariableDescriptorPreview;
            }
            catch (Exception ex)
            {
                StatusBlock.Text = $"error: {ex.Message}";
                _model = null;
            }
        }

        /// <summary>
        /// Trigger file picker and image evaluation
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private async void ButtonRun_Click(object sender, RoutedEventArgs e)
        {
            ButtonRun.IsEnabled = false;
            UIPreviewImage.Source = null;
            try
            {
                // Load the model
                await LoadModelAsync();

                // Trigger file picker to select an image file
                FileOpenPicker fileOpenPicker = new FileOpenPicker();
                fileOpenPicker.SuggestedStartLocation = PickerLocationId.PicturesLibrary;
                fileOpenPicker.FileTypeFilter.Add(".jpg");
                fileOpenPicker.FileTypeFilter.Add(".png");
                fileOpenPicker.ViewMode = PickerViewMode.Thumbnail;
                StorageFile selectedStorageFile = await fileOpenPicker.PickSingleFileAsync();

                SoftwareBitmap softwareBitmap;
                using (IRandomAccessStream stream = await selectedStorageFile.OpenAsync(FileAccessMode.Read))
                {
                    // Create the decoder from the stream 
                    BitmapDecoder decoder = await BitmapDecoder.CreateAsync(stream);

                    // Get the SoftwareBitmap representation of the file in BGRA8 format
                    softwareBitmap = await decoder.GetSoftwareBitmapAsync();
                    softwareBitmap = SoftwareBitmap.Convert(softwareBitmap, BitmapPixelFormat.Bgra8, BitmapAlphaMode.Premultiplied);
                }

                // Display the image
                SoftwareBitmapSource imageSource = new SoftwareBitmapSource();
                await imageSource.SetBitmapAsync(softwareBitmap);
                UIPreviewImage.Source = imageSource;

                // Encapsulate the image within a VideoFrame to be bound and evaluated
                VideoFrame inputImage = VideoFrame.CreateWithSoftwareBitmap(softwareBitmap);

                await EvaluateVideoFrameAsync(inputImage);
            }
            catch (Exception ex)
            {
                StatusBlock.Text = $"error: {ex.Message}";
                ButtonRun.IsEnabled = true;
            }
        }

        /// <summary>
        /// Evaluate the VideoFrame passed in as arg
        /// </summary>
        /// <param name="inputFrame"></param>
        /// <returns></returns>
        private async Task EvaluateVideoFrameAsync(VideoFrame inputFrame)
        {
            if (inputFrame != null)
            {
                try
                {
                    // Create bindings for the input and output buffer
                    LearningModelBindingPreview binding = new LearningModelBindingPreview(_model as LearningModelPreview);
                    binding.Bind(_inputImageDescription.Name, inputFrame);
                    binding.Bind(_outputTensorDescription.Name, _outputVariableList);

                    // Process the frame with the model, and time the operation
                    modeltime_w.Restart();
                    LearningModelEvaluationResultPreview results = await _model.EvaluateAsync(binding, "test");
                    modeltime_w.Stop();

                    List<float> resultProbabilities = results.Outputs[_outputTensorDescription.Name] as List<float>;

                    // Find the result of the evaluation in the bound output (the top classes detected with the max confidence)
                    List<float> topProbabilities = new List<float>() { 0.0f, 0.0f, 0.0f };
                    List<int> topProbabilityLabelIndexes = new List<int>() { 0, 0, 0 };
                    for (int i = 0; i < resultProbabilities.Count(); i++)
                    {
                        for (int j = 0; j < 3; j++)
                        {
                            if (resultProbabilities[i] > topProbabilities[j])
                            {
                                topProbabilityLabelIndexes[j] = i;
                                topProbabilities[j] = resultProbabilities[i];
                                break;
                            }
                        }
                    }

                    // Display the result
                    var runtimemS = modeltime_w.ElapsedMilliseconds;

                    string message = $"Runtime {runtimemS}mS Predominant objects detected are:";
                    for (int i = 0; i < 3; i++)
                    {
                        message += $"\n\"{ _labels[topProbabilityLabelIndexes[i]]}\" with confidence of { topProbabilities[i]}";
                    }
                    StatusBlock.Text = message;
                }
                catch (Exception ex)
                {
                    StatusBlock.Text = $"error: {ex.Message}";
                }

                ButtonRun.IsEnabled = true;
            }
        }
    }
}
