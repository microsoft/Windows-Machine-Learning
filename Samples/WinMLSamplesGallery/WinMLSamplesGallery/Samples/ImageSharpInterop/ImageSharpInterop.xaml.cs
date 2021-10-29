using Microsoft.AI.MachineLearning;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Media;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Windows.Foundation.Metadata;
using Windows.Graphics.Imaging;
using Windows.Media;
using Windows.Storage;
using Windows.UI;
using WinMLSamplesGallery.Common;
using WinMLSamplesGallery.Controls;
using SixLabors.ImageSharp;
using SixLabors.ImageSharp.PixelFormats;
using ImageSharpExtensrions;
using SixLabors.ImageSharp.Processing;

namespace WinMLSamplesGallery.Samples
{
    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class ImageSharpInterop : Page
    {
        const long BatchSize = 1;
        const long TopK = 10;
        const long Height = 224;
        const long Width = 224;
        const long Channels = 4;

        private LearningModelSession _inferenceSession;
        private LearningModelSession _tensorizationSession;
        private LearningModelSession _postProcessingSession;

        private static Dictionary<long, string> _imagenetLabels;

        private Image<Bgra32> CurrentImage { get; set; }

#pragma warning disable CA1416 // Validate platform compatibility
        private LearningModelDeviceKind SelectedDeviceKind
        {
            get
            {
                return (DeviceComboBox.SelectedIndex == 0) ?
                            LearningModelDeviceKind.Cpu :
                            LearningModelDeviceKind.DirectXHighPerformance;
            }
        }
#pragma warning restore CA1416 // Validate platform compatibility

        public ImageSharpInterop()
        {
            this.InitializeComponent();

            _imagenetLabels = LoadLabels("ms-appx:///InputData/sysnet.txt");
            var tensorizationModel = TensorizationModels.BasicTensorization(Height, Width, BatchSize, Channels, Height, Width, "nearest");
            _tensorizationSession = CreateLearningModelSession(tensorizationModel, SelectedDeviceKind);
            _inferenceSession = CreateLearningModelSession("ms-appx:///Models/squeezenet1.1-7.onnx");
            _postProcessingSession = CreateLearningModelSession(TensorizationModels.SoftMaxThenTopK(TopK));

        }

#pragma warning disable CA1416 // Validate platform compatibility
        private (IEnumerable<string>, IReadOnlyList<float>) Classify(Image<Bgra32> image, float angle)
        {
            long start, stop;
            PerformanceMetricsMonitor.ClearLog();

            // Tensorize
            start = HighResolutionClock.UtcNow();
            image.Mutate(ctx => ctx.Rotate(angle));
            var resizeOptions = new ResizeOptions()
            {
                Mode = ResizeMode.Crop,
                Size = new SixLabors.ImageSharp.Size((int)Width, (int)Height)
            };
            image.Mutate(ctx => ctx.Resize(resizeOptions));
            object input = image.AsTensor();
            var tensorizationResults = Evaluate(_tensorizationSession, input);
            object tensorizedOutput = tensorizationResults.Outputs.First().Value;
            stop = HighResolutionClock.UtcNow();
            var tensorizeDuration = HighResolutionClock.DurationInMs(start, stop);

            // Inference
            start = HighResolutionClock.UtcNow();
            var inferenceResults = Evaluate(_inferenceSession, tensorizedOutput);
            var inferenceOutput = inferenceResults.Outputs.First().Value;
            stop = HighResolutionClock.UtcNow();
            var inferenceDuration = HighResolutionClock.DurationInMs(start, stop);

            // PostProcess
            start = HighResolutionClock.UtcNow();
            var postProcessedOutputs = Evaluate(_postProcessingSession, inferenceOutput);
            var topKValues = (TensorFloat)postProcessedOutputs.Outputs["TopKValues"];
            var topKIndices = (TensorInt64Bit)postProcessedOutputs.Outputs["TopKIndices"];

            // Return results
            var probabilities = topKValues.GetAsVectorView();
            var indices = topKIndices.GetAsVectorView();
            var labels = indices.Select((index) => _imagenetLabels[index]);
            stop = HighResolutionClock.UtcNow();
            var postProcessDuration = HighResolutionClock.DurationInMs(start, stop);

            PerformanceMetricsMonitor.Log("Tensorize", tensorizeDuration);
            PerformanceMetricsMonitor.Log("Pre-process", 0);
            PerformanceMetricsMonitor.Log("Inference", inferenceDuration);
            PerformanceMetricsMonitor.Log("Post-process", postProcessDuration);

            RenderingHelpers.BindSoftwareBitmapToImageControl(InputImage, image.AsSoftwareBitmap());
            image.Dispose();

            return (labels, probabilities);
        }

        private static LearningModelEvaluationResult Evaluate(LearningModelSession session, object input)
        {
            // Create the binding
            var binding = new LearningModelBinding(session);

            // Create an emoty output, that will keep the output resources on the GPU
            // It will be chained into a the post processing on the GPU as well
            var output = TensorFloat.Create();

            // Bind inputs and outputs
            // For squeezenet these evaluate to "data", and "squeezenet0_flatten0_reshape0"
            string inputName = session.Model.InputFeatures[0].Name;
            string outputName = session.Model.OutputFeatures[0].Name;
            binding.Bind(inputName, input);

            var outputBindProperties = new PropertySet();
            outputBindProperties.Add("DisableTensorCpuSync", PropertyValue.CreateBoolean(true));
            binding.Bind(outputName, output, outputBindProperties);

            // Evaluate
            return session.Evaluate(binding, "");
        }

        private LearningModelSession CreateLearningModelSession(string modelPath)
        {
            var model = CreateLearningModel(modelPath);
            var session =  CreateLearningModelSession(model);
            return session;
        }

        private LearningModelSession CreateLearningModelSession(LearningModel model, Nullable<LearningModelDeviceKind> kind = null)
        {
            var device = new LearningModelDevice(kind ?? SelectedDeviceKind);
            var options = new LearningModelSessionOptions()
            {
                CloseModelOnSessionCreation = true // Close the model to prevent extra memory usage
            };
            var session = new LearningModelSession(model, device, options);
            return session;
        }

        private static LearningModel CreateLearningModel(string modelPath)
        {
            var uri = new Uri(modelPath);
            var file = StorageFile.GetFileFromApplicationUriAsync(uri).GetAwaiter().GetResult();
            return LearningModel.LoadFromStorageFileAsync(file).GetAwaiter().GetResult();
        }
#pragma warning restore CA1416 // Validate platform compatibility

        private static Dictionary<long, string> LoadLabels(string csvFile)
        {
            var file = StorageFile.GetFileFromApplicationUriAsync(new Uri(csvFile)).GetAwaiter().GetResult();
            var text = Windows.Storage.FileIO.ReadTextAsync(file).GetAwaiter().GetResult();
            var labels = new Dictionary<long, string>();
            var records = text.Split(Environment.NewLine);
            foreach (var record in records)
            {
                var fields = record.Split(",", 2);
                if (fields.Length == 2)
                {
                    var index = long.Parse(fields[0]);
                    labels[index] = fields[1];
                }
            }
            return labels;
        }

        private void TryPerformInference(bool reloadImages = true)
        {
            if (CurrentImage != null)
            {
                // Classify
                var angle = (float)RotationSlider.Value;
                var (labels, probabilities) = Classify(CurrentImage.Clone(), angle);

                // Render the classification and probabilities
                RenderInferenceResults(labels, probabilities);
            }
        }

        private void RenderInferenceResults(IEnumerable<string> labels, IReadOnlyList<float> probabilities)
        {
            var indices = Enumerable.Range(1, probabilities.Count);
            var zippedResults = indices.Zip(labels.Zip(probabilities));
            var results = zippedResults.Select(
                (zippedResult) =>
                    new Controls.Prediction {
                        Index = zippedResult.First,
                        Name = zippedResult.Second.First.Trim(new char[] { ',' }),
                        Probability = zippedResult.Second.Second.ToString("E4")
                    });
            InferenceResults.ItemsSource = results;
            InferenceResults.SelectedIndex = 0;
        }

        private void OpenButton_Clicked(object sender, RoutedEventArgs e)
        {
            var storageFile = ImageHelper.PickImageFiles();
            if (storageFile != null)
            {
                BasicGridView.SelectedItem = null;
                SetCurrentImage(storageFile.Path);
                TryPerformInference();
            }
        }

        private void SampleInputsGridView_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            var gridView = (GridView)sender;
            var thumbnail = (Thumbnail)gridView.SelectedItem;
            if (thumbnail != null)
            {
                var file = StorageFile.GetFileFromApplicationUriAsync(new Uri(thumbnail.ImageUri)).GetAwaiter().GetResult();
                SetCurrentImage(file.Path);
                TryPerformInference();
            }
        }

        private void SetCurrentImage(string path)
        {
            if (CurrentImage != null) CurrentImage.Dispose();

            RotationSlider.IsEnabled = true;

            CurrentImage = SixLabors.ImageSharp.Image.Load<Bgra32>(path);
            RenderingHelpers.BindSoftwareBitmapToImageControl(InputImage, CurrentImage.AsSoftwareBitmap());
        }

        private void DeviceComboBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            TryPerformInference();
        }

        private void RotationSlider_ValueChanged(object sender, Microsoft.UI.Xaml.Controls.Primitives.RangeBaseValueChangedEventArgs e)
        {
            TryPerformInference();
        }
    }
}
