using Microsoft.AI.MachineLearning;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Data;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Windows.Foundation.Metadata;
using Windows.Graphics.Imaging;
using Windows.Media;
using Windows.Storage;
using WinMLSamplesGallery.Common;
using WinMLSamplesGallery.Controls;

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

            var allModels = new List<ClassifierViewModel> {
                new ClassifierViewModel { Tag = Classifier.SqueezeNet, Title = "SqueezeNet" },
                new ClassifierViewModel { Tag = Classifier.DenseNet121, Title = "DenseNet-121" },
                new ClassifierViewModel { Tag = Classifier.ShuffleNet_V1, Title = "ShuffleNet_V1" },
                new ClassifierViewModel { Tag = Classifier.EfficientNetLite4, Title = "EfficientNet-Lite4" },

#if USE_LARGE_MODELS
                new ClassifierViewModel { Tag = Classifier.MobileNet, Title="MobileNet" },
                new ClassifierViewModel { Tag = Classifier.GoogleNet, Title="GoogleNet" },
                new ClassifierViewModel { Tag = Classifier.Inception_V1, Title="Inception_V1" },
                new ClassifierViewModel { Tag = Classifier.Inception_V2, Title="Inception_V2" },
                new ClassifierViewModel { Tag = Classifier.ShuffleNet_V2, Title="ShuffleNet_V2" },
                new ClassifierViewModel { Tag = Classifier.RCNN_ILSVRC13, Title="RCNN_ILSVRC13" },
                new ClassifierViewModel { Tag = Classifier.ResNet, Title="ResNet" },
                new ClassifierViewModel { Tag = Classifier.VGG, Title="VGG" },
                new ClassifierViewModel { Tag = Classifier.AlexNet, Title="AlexNet" },
                new ClassifierViewModel { Tag = Classifier.CaffeNet, Title="CaffeNet" },
                new ClassifierViewModel { Tag = Classifier.ZFNet512, Title="ZFNet-512" },
#endif
                };
            AllModelsGrid.ItemsSource = allModels;
            AllModelsGrid.SelectRange(new ItemIndexRange(0, 1));
            AllModelsGrid.SelectionChanged += AllModelsGrid_SelectionChanged;

            BasicGridView.SelectedIndex = 0;
        }

        private void EnsureInitialized()
        {
            if (_modelDictionary == null)
            {
                var installPath = Windows.ApplicationModel.Package.Current.InstalledLocation.Path;
                _modelDictionary = new Dictionary<Classifier, string>{
                    { Classifier.DenseNet121,       Path.Combine(installPath, "Models\\densenet-9.onnx") },
                    { Classifier.EfficientNetLite4, Path.Combine(installPath, "Models\\efficientnet-lite4-11.onnx") },
                    { Classifier.ShuffleNet_V1,     Path.Combine(installPath, "Models\\shufflenet-9.onnx") },
                    { Classifier.SqueezeNet,        Path.Combine(installPath, "Models\\squeezenet1.1-7.onnx") },
#if USE_LARGE_MODELS
                    // Large Models
                    { Classifier.AlexNet,           Path.Combine(installPath, "LargeModels\\bvlcalexnet-9.onnx") },
                    { Classifier.CaffeNet,          Path.Combine(installPath, "LargeModels\\caffenet-9.onnx") },
                    { Classifier.GoogleNet,         Path.Combine(installPath, "LargeModels\\googlenet-9.onnx") },
                    { Classifier.Inception_V1,      Path.Combine(installPath, "LargeModels\\inception-v1-9.onnx") },
                    { Classifier.Inception_V2,      Path.Combine(installPath, "LargeModels\\inception-v2-9.onnx") },
                    { Classifier.MobileNet,         Path.Combine(installPath, "LargeModels\\mobilenetv2-7.onnx" },
                    { Classifier.ShuffleNet_V2,     Path.Combine(installPath, "LargeModels\\shufflenet-v2-10.onnx") },
                    { Classifier.RCNN_ILSVRC13,     Path.Combine(installPath, "LargeModels\\rcnn-ilsvrc13-9.onnx") },
                    { Classifier.ResNet,            Path.Combine(installPath, "LargeModels\\resnet50-caffe2-v1-9.onnx") },
                    { Classifier.VGG,               Path.Combine(installPath, "LargeModels\\vgg19-7.onnx") },
                    { Classifier.ZFNet512,          Path.Combine(installPath, "LargeModels\\zfnet512-9.onnx") },
#endif
                };
            }

            if (_postProcessorDictionary == null)
            {
                _postProcessorDictionary = new Dictionary<Classifier, Func<LearningModel>>{
                    { Classifier.DenseNet121,       () => TensorizationModels.ReshapeThenSoftmaxThenTopK(
                                                              new long[] { BatchSize, ClassificationLabels.ImageNet.Count, 1, 1 },
                                                              TopK,
                                                              BatchSize,
                                                              ClassificationLabels.ImageNet.Count) },
                    { Classifier.EfficientNetLite4, () => TensorizationModels.SoftMaxThenTopK(TopK) },
                    { Classifier.ShuffleNet_V1,     () => TensorizationModels.TopK(TopK) },
                    { Classifier.SqueezeNet,        () => TensorizationModels.SoftMaxThenTopK(TopK) },
#if USE_LARGE_MODELS
                    // Large Models
                    { Classifier.AlexNet,           () => TensorizationModels.TopK(TopK) },
                    { Classifier.CaffeNet,          () => TensorizationModels.TopK(TopK) },
                    { Classifier.GoogleNet,         () => TensorizationModels.TopK(TopK) }, //chop
                    { Classifier.Inception_V1,      () => TensorizationModels.TopK(TopK) },//chop
                    { Classifier.Inception_V2,      () => TensorizationModels.TopK(TopK) }, //chop
                    { Classifier.MobileNet,         () => TensorizationModels.SoftMaxThenTopK(TopK) }, //cchop
                    { Classifier.ShuffleNet_V2,     () => TensorizationModels.SoftMaxThenTopK(TopK) }, //chop
                    { Classifier.RCNN_ILSVRC13,     () => TensorizationModels.TopK(TopK) },
                    { Classifier.ResNet,            () => TensorizationModels.SoftMaxThenTopK(TopK) },
                    { Classifier.VGG,               () => TensorizationModels.SoftMaxThenTopK(TopK) },
                    { Classifier.ZFNet512,          () => TensorizationModels.TopK(TopK) },
#endif
                };
            }

            if (_preProcessorDictionary == null)
            {
                // Preprocessing values are described in the ONNX Model Zoo:
                // https://github.com/onnx/models/tree/master/vision/classification/mobilenet
                _preProcessorDictionary = new Dictionary<Classifier, Func<LearningModel>>{
                    { Classifier.DenseNet121,       () => TensorizationModels.Normalize0_1ThenZScore(
                                                              Height, Width, Channels,
                                                              new float[] { 0.485f, 0.456f, 0.406f },
                                                              new float[] { 0.229f, 0.224f, 0.225f}) },
                    { Classifier.EfficientNetLite4, () => TensorizationModels.NormalizeMinusOneToOneThenTransposeNHWC() },
                    { Classifier.ShuffleNet_V1,     () => TensorizationModels.Normalize0_1ThenZScore(
                                                              Height, Width, Channels,
                                                              new float[] { 0.485f, 0.456f, 0.406f },
                                                              new float[] { 0.229f, 0.224f, 0.225f}) },
                    { Classifier.SqueezeNet,        null }, // No preprocessing required
#if USE_LARGE_MODELS
                    // Large Models
                    { Classifier.AlexNet,           null }, // No preprocessing required
                    { Classifier.CaffeNet,          null }, // No preprocessing required
                    { Classifier.GoogleNet,         null },
                    { Classifier.Inception_V1,      null }, // No preprocessing required
                    { Classifier.Inception_V2,      null }, // ????
                    { Classifier.MobileNet,         () => TensorizationModels.Normalize0_1ThenZScore(
                                                              Height, Width, Channels,
                                                              new float[] { 0.485f, 0.456f, 0.406f },
                                                              new float[] { 0.229f, 0.224f, 0.225f}) },
                    { Classifier.RCNN_ILSVRC13,     null }, // No preprocessing required
                    { Classifier.ResNet,            () => TensorizationModels.Normalize0_1ThenZScore(
                                                              224, 224, 4,
                                                              new float[] { 0.485f, 0.456f, 0.406f },
                                                              new float[] { 0.229f, 0.224f, 0.225f}) },
                    { Classifier.ShuffleNet_V2,     () => TensorizationModels.Normalize0_1ThenZScore(
                                                              Height, Width, Channels,
                                                              new float[] { 0.485f, 0.456f, 0.406f },
                                                              new float[] { 0.229f, 0.224f, 0.225f}) },
                    { Classifier.VGG,               () => TensorizationModels.Normalize0_1ThenZScore(
                                                              224, 224, 4,
                                                              new float[] { 0.485f, 0.456f, 0.406f },
                                                              new float[] { 0.229f, 0.224f, 0.225f}) },
                    { Classifier.ZFNet512,          null }, // No preprocessing required
#endif
                };
            }

            InitializeWindowsMachineLearning();
        }

        private void InitializeWindowsMachineLearning()
        {
            var tensorizationModel = TensorizationModels.BasicTensorization(
                                         Height, Width,
                                         BatchSize, Channels, CurrentImageDecoder.PixelHeight, CurrentImageDecoder.PixelWidth,
                                         "nearest");
            _tensorizationSession = CreateLearningModelSession(tensorizationModel, LearningModelDeviceKind.Cpu);

            var model = SelectedModel;
            if (model != CurrentModel)
            {
                var modelPath = _modelDictionary[model];
                var inferenceModel = LearningModel.LoadFromFilePath(modelPath);
                _inferenceSession = CreateLearningModelSession(inferenceModel);

                var preProcessor = _preProcessorDictionary[model];
                var hasPreProcessor = preProcessor != null;
                _preProcessingSession = hasPreProcessor ? CreateLearningModelSession(preProcessor()) : null;

                var postProcessor = _postProcessorDictionary[model];
                var hasPostProcessor = postProcessor != null;
                _postProcessingSession = hasPostProcessor ? CreateLearningModelSession(postProcessor()) : null;

                if (model == Classifier.RCNN_ILSVRC13)
                {
                    _labels = ClassificationLabels.ILSVRC2013;
                }
                else
                {
                    _labels = ClassificationLabels.ImageNet;
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
                var buffer = bytes.AsBuffer();
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
            // It will be chained into post processing on the GPU as well
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

#pragma warning restore CA1416 // Validate platform compatibility
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
                        Probability = zippedResult.Second.Second.ToString("P")
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
