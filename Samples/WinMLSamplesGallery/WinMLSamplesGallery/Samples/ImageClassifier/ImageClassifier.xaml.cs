using System;
using System.Collections.Generic;
using System.Linq;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Data;
using Windows.Storage;
using Windows.Graphics.Imaging;
using Windows.Media;
using Microsoft.AI.MachineLearning;
using WinMLSamplesGallery.Common;
using WinMLSamplesGallery.Controls;
using Windows.Foundation.Metadata;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.Foundation.Collections;
using Windows.Foundation;

namespace WinMLSamplesGallery.Samples
{
    public enum Classifier
    {
        NotSet = 0,
        MobileNet,
        ResNet,
        SqueezeNet,
        VGG,
        AlexNet,
        GoogleNet,
        CaffeNet,
        RCNN_ILSVRC13,
        DenseNet121,
        Inception_V1,
        Inception_V2,
        ShuffleNet_V1,
        ShuffleNet_V2,
        ZFNet512,
        EfficientNetLite4,
    }

    public sealed class ClassifierViewModel
    {
        public string Title { get; set; }
        public Classifier Tag { get; set; }
    }

    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class ImageClassifier : Page
    {
        const long BatchSize = 1;
        const long TopK = 10;
        const long Height = 224;
        const long Width = 224;
        const long Channels = 4;

        private LearningModelSession _inferenceSession;
        private LearningModelSession _postProcessingSession;
        private LearningModelSession _preProcessingSession;
        private LearningModelSession _tensorizationSession;

        private Dictionary<Classifier, string> _modelDictionary;
        private Dictionary<Classifier, Func<LearningModel>> _postProcessorDictionary;
        private Dictionary<Classifier, Func<LearningModel>> _preProcessorDictionary;

        private static Dictionary<long, string> _labels;
        private static Dictionary<long, string> _imagenetLabels;
        private static Dictionary<long, string> _ilsvrc2013Labels;


        private BitmapDecoder CurrentImageDecoder { get; set; }

        private Classifier CurrentModel { get; set; }

        private Classifier SelectedModel
        {
            get
            {
                if (AllModelsGrid == null || AllModelsGrid.SelectedItem == null) {
                    return Classifier.NotSet;
                }
                var viewModel = (ClassifierViewModel)AllModelsGrid.SelectedItem;
                return viewModel.Tag;
            }
        }

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

        public ImageClassifier()
        {
            this.InitializeComponent();

            CurrentImageDecoder = null;
            CurrentModel = Classifier.NotSet;
            AllModelsGrid.SelectRange(new ItemIndexRange(0, 1));
            AllModelsGrid.SelectionChanged += AllModelsGrid_SelectionChanged;
        }

        private void EnsureInitialized()
        {
            if (_imagenetLabels == null)
            {
                _imagenetLabels = LoadLabels("ms-appx:///InputData/sysnet.txt");
            }

            if (_ilsvrc2013Labels == null)
            {
                _ilsvrc2013Labels = LoadLabels("ms-appx:///InputData/ilsvrc2013.txt");
            }

            if (_modelDictionary == null)
            {
                _modelDictionary = new Dictionary<Classifier, string>{
                    { Classifier.SqueezeNet,        "ms-appx:///Models/squeezenet1.1-7.onnx" },
                    { Classifier.MobileNet,         "ms-appx:///Models/mobilenetv2-7.onnx" },
                    { Classifier.GoogleNet,         "ms-appx:///Models/googlenet-9.onnx"},
                    { Classifier.DenseNet121,       "ms-appx:///Models/densenet-9.onnx"},
                    { Classifier.Inception_V1,      "ms-appx:///Models/inception-v1-9.onnx"},
                    { Classifier.Inception_V2,      "ms-appx:///Models/inception-v2-9.onnx"},
                    { Classifier.ShuffleNet_V1,     "ms-appx:///Models/shufflenet-9.onnx"},
                    { Classifier.ShuffleNet_V2,     "ms-appx:///Models/shufflenet-v2-10.onnx"},
                    { Classifier.EfficientNetLite4, "ms-appx:///Models/efficientnet-lite4-11.onnx"},
                    // Large Models
                    { Classifier.AlexNet,           "ms-appx:///LargeModels/bvlcalexnet-9.onnx"},
                    { Classifier.CaffeNet,          "ms-appx:///LargeModels/caffenet-9.onnx"},
                    { Classifier.RCNN_ILSVRC13,     "ms-appx:///LargeModels/rcnn-ilsvrc13-9.onnx"},
                    { Classifier.ResNet,            "ms-appx:///LargeModels/resnet50-caffe2-v1-9.onnx"},
                    { Classifier.VGG,               "ms-appx:///LargeModels/vgg19-7.onnx"},
                    { Classifier.ZFNet512,          "ms-appx:///LargeModels/zfnet512-9.onnx"},
                };
            }

            if (_postProcessorDictionary == null)
            {
                _postProcessorDictionary = new Dictionary<Classifier, Func<LearningModel>>{
                    { Classifier.SqueezeNet,        () => TensorizationModels.SoftMaxThenTopK(TopK) },
                    { Classifier.MobileNet,         () => TensorizationModels.SoftMaxThenTopK(TopK) },
                    { Classifier.GoogleNet,         () => TensorizationModels.TopK(TopK) },
                    { Classifier.DenseNet121,       () => TensorizationModels.ReshapeThenSoftmaxThenTopK(new long[] { BatchSize, _imagenetLabels.Count, 1, 1 },
                                                                                                    TopK,
                                                                                                    BatchSize,
                                                                                                    _imagenetLabels.Count) },
                    { Classifier.Inception_V1,      () => TensorizationModels.TopK(TopK) },
                    { Classifier.Inception_V2,      () => TensorizationModels.TopK(TopK) },
                    { Classifier.ShuffleNet_V1,     () => TensorizationModels.TopK(TopK) },
                    { Classifier.ShuffleNet_V2,     () => TensorizationModels.SoftMaxThenTopK(TopK) },
                    { Classifier.EfficientNetLite4, () => TensorizationModels.SoftMaxThenTopK(TopK) },
                    // Large Models
                    { Classifier.AlexNet,           () => TensorizationModels.TopK(TopK) },
                    { Classifier.CaffeNet,          () => TensorizationModels.TopK(TopK) },
                    { Classifier.RCNN_ILSVRC13,     () => TensorizationModels.TopK(TopK) },
                    { Classifier.ResNet,            () => TensorizationModels.SoftMaxThenTopK(TopK) },
                    { Classifier.VGG,               () => TensorizationModels.SoftMaxThenTopK(TopK) },
                    { Classifier.ZFNet512,          () => TensorizationModels.TopK(TopK) },
                };
            }

            if (_preProcessorDictionary == null)
            {
                // Preprocessing values are described in the ONNX Model Zoo:
                // https://github.com/onnx/models/tree/master/vision/classification/mobilenet
                _preProcessorDictionary = new Dictionary<Classifier, Func<LearningModel>>{
                    { Classifier.SqueezeNet,        null }, // No preprocessing required
                    { Classifier.MobileNet,         () => TensorizationModels.Normalize0_1ThenZScore(Height, Width, Channels,
                                                                                                new float[] { 0.485f, 0.456f, 0.406f },
                                                                                                new float[] { 0.229f, 0.224f, 0.225f}) },
                    { Classifier.GoogleNet,         null },
                    { Classifier.DenseNet121,       () => TensorizationModels.Normalize0_1ThenZScore(Height, Width, Channels,
                                                                                                new float[] { 0.485f, 0.456f, 0.406f },
                                                                                                new float[] { 0.229f, 0.224f, 0.225f}) },
                    { Classifier.Inception_V1,      null }, // No preprocessing required
                    { Classifier.Inception_V2,      null }, // ????
                    { Classifier.ShuffleNet_V1,     () => TensorizationModels.Normalize0_1ThenZScore(Height, Width, Channels,
                                                                                                new float[] { 0.485f, 0.456f, 0.406f },
                                                                                                new float[] { 0.229f, 0.224f, 0.225f}) },
                    { Classifier.ShuffleNet_V2,     () => TensorizationModels.Normalize0_1ThenZScore(Height, Width, Channels,
                                                                                                new float[] { 0.485f, 0.456f, 0.406f },
                                                                                                new float[] { 0.229f, 0.224f, 0.225f}) },
                    { Classifier.EfficientNetLite4, () => TensorizationModels.NormalizeMinusOneToOneThenTransposeNHWC() },
                    // Large Models
                    { Classifier.AlexNet,           null }, // No preprocessing required
                    { Classifier.CaffeNet,          null }, // No preprocessing required
                    { Classifier.RCNN_ILSVRC13,     null }, // No preprocessing required
                    { Classifier.ResNet,            () => TensorizationModels.Normalize0_1ThenZScore(224, 224, 4,
                                                                                                new float[] { 0.485f, 0.456f, 0.406f },
                                                                                                new float[] { 0.229f, 0.224f, 0.225f}) },
                    { Classifier.VGG,               () => TensorizationModels.Normalize0_1ThenZScore(224, 224, 4,
                                                                                                new float[] { 0.485f, 0.456f, 0.406f },
                                                                                                new float[] { 0.229f, 0.224f, 0.225f}) },
                    { Classifier.ZFNet512,          null }, // No preprocessing required
                };
            }

            InitializeWindowsMachineLearning();
        }

        private void InitializeWindowsMachineLearning()
        {
            _tensorizationSession =
                CreateLearningModelSession(
                    TensorizationModels.BasicTensorization(
                        Height, Width,
                        BatchSize, Channels, CurrentImageDecoder.PixelHeight, CurrentImageDecoder.PixelWidth,
                        "nearest"),
                        LearningModelDeviceKind.Cpu);

            var model = SelectedModel;
            if (model != CurrentModel)
            {
                var modelPath = _modelDictionary[model];
                _inferenceSession = CreateLearningModelSession(modelPath);

                var preProcessor = _preProcessorDictionary[model];
                var hasPreProcessor = preProcessor != null;
                _preProcessingSession = hasPreProcessor ? CreateLearningModelSession(preProcessor()) : null;

                var postProcessor = _postProcessorDictionary[model];
                var hasPostProcessor = postProcessor != null;
                _postProcessingSession = hasPostProcessor ? CreateLearningModelSession(postProcessor()) : null;

                if (model == Classifier.RCNN_ILSVRC13)
                {
                    _labels = _ilsvrc2013Labels;
                }
                else
                {
                    _labels = _imagenetLabels;
                }

                CurrentModel = model;
            }
        }

#pragma warning disable CA1416 // Validate platform compatibility
        private (IEnumerable<string>, IReadOnlyList<float>) Classify(BitmapDecoder decoder)
        {
            long start, stop;
            PerformanceMetricsMonitor.ClearLog();

            LearningModelSession tensorizationSession = null;

            start = HighResolutionClock.UtcNow();
            object input = null;
            if (ApiInformation.IsTypePresent("Windows.Media.VideoFrame"))
            {
                var softwareBitmap = decoder.GetSoftwareBitmapAsync().GetAwaiter().GetResult();
                input = VideoFrame.CreateWithSoftwareBitmap(softwareBitmap);
            }
            else
            {
                var pixelDataProvider = decoder.GetPixelDataAsync().GetAwaiter().GetResult();
                var bytes = pixelDataProvider.DetachPixelData();
                var buffer = bytes.AsBuffer(); // Need to make this 0 copy...
                input = TensorUInt8Bit.CreateFromBuffer(new long[] { 1, buffer.Length }, buffer);

                tensorizationSession = _tensorizationSession;
            }
            stop = HighResolutionClock.UtcNow();
            var pixelAccessDuration = HighResolutionClock.DurationInMs(start, stop);

            // Tensorize
            start = HighResolutionClock.UtcNow();
            object tensorizedOutput = input;
            if (tensorizationSession != null)
            {
                var tensorizationResults = Evaluate(tensorizationSession, input);
                tensorizedOutput = tensorizationResults.Outputs.First().Value;
            }
            stop = HighResolutionClock.UtcNow();
            var tensorizeDuration = HighResolutionClock.DurationInMs(start, stop);

            // PreProcess
            start = HighResolutionClock.UtcNow();
            object preProcessedOutput = tensorizedOutput;
            if (_preProcessingSession != null)
            {
                var preProcessedResults = Evaluate(_preProcessingSession, tensorizedOutput);
                preProcessedOutput = preProcessedResults.Outputs.First().Value;
            }
            stop = HighResolutionClock.UtcNow();
            var preprocessDuration = HighResolutionClock.DurationInMs(start, stop);

            // Inference
            start = HighResolutionClock.UtcNow();
            var inferenceResults = Evaluate(_inferenceSession, preProcessedOutput);
            var inferenceOutput = inferenceResults.Outputs.First().Value;
            stop = HighResolutionClock.UtcNow();
            var inferenceDuration = HighResolutionClock.DurationInMs(start, stop);

            // PostProcess
            start = HighResolutionClock.UtcNow();
            var postProcessedOutputs = Evaluate(_postProcessingSession, inferenceOutput);
            var topKValues = (TensorFloat)postProcessedOutputs.Outputs["TopKValues"] ;
            var topKIndices = (TensorInt64Bit)postProcessedOutputs.Outputs["TopKIndices"];

            // Return results
            var probabilities = topKValues.GetAsVectorView();
            var indices = topKIndices.GetAsVectorView();
            var labels = indices.Select((index) => _labels[index]);
            stop = HighResolutionClock.UtcNow();
            var postProcessDuration = HighResolutionClock.DurationInMs(start, stop);

            PerformanceMetricsMonitor.Log("Pixel Access (CPU)", pixelAccessDuration);
            PerformanceMetricsMonitor.Log("Tensorize (CPU)", tensorizeDuration);
            PerformanceMetricsMonitor.Log("Pre-process", preprocessDuration);
            PerformanceMetricsMonitor.Log("Inference", inferenceDuration);
            PerformanceMetricsMonitor.Log("Post-process", postProcessDuration);

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

        private void TryPerformInference()
        {
            if (CurrentImageDecoder != null)
            {
                EnsureInitialized();

                // Draw the image to classify in the Image control
                var softwareBitmap = CurrentImageDecoder.GetSoftwareBitmapAsync().GetAwaiter().GetResult();
                RenderingHelpers.BindSoftwareBitmapToImageControl(InputImage, softwareBitmap);

                // Classify the current image
                var (labels, probabilities) = Classify(CurrentImageDecoder);

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
            var bitmap = ImageHelper.PickImageFileAsBitmapDecoder();
            if (bitmap != null)
            {
                BasicGridView.SelectedItem = null;
                CurrentImageDecoder = bitmap;
                TryPerformInference();
            }
        }

        private void SampleInputsGridView_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            var gridView = (GridView)sender;
            var thumbnail = (Thumbnail)gridView.SelectedItem;
            if (thumbnail != null)
            {
                CurrentImageDecoder = ImageHelper.CreateBitmapDecoderFromPath(thumbnail.ImageUri);
                TryPerformInference();
            }
        }

        private void AllModelsGrid_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            TryPerformInference();
        }

        private void DeviceComboBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            TryPerformInference();
        }
    }
}
