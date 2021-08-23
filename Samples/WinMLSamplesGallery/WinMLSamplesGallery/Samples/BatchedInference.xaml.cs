using System;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using System.Runtime.InteropServices;
using Windows.Storage.Pickers;
using Windows.Storage;
using Windows.Graphics.Imaging;
using Windows.Media;
using Microsoft.AI.MachineLearning;
using Microsoft.UI.Xaml.Media.Imaging;

namespace WinMLSamplesGallery.Samples
{
    public sealed class InferenceResult
    {
        public string Label { get; set; }
        public string PreprocessTime { get; set; }
        public string InferenceTime { get; set; }
        public string PostprocessTime { get; set; }
    }

    public sealed class TotalMetricTime
    {
        public string Metric { get; set; }
        public string TotalTime { get; set; }
    }

    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class BatchedInference : Page
    {
        [DllImport("user32.dll", ExactSpelling = true, CharSet = CharSet.Auto, PreserveSig = true, SetLastError = false)]
        public static extern IntPtr GetActiveWindow();

        private static Dictionary<long, string> labels_;
        private LearningModelSession inferenceSession_;
        private LearningModelSession preProcessingSession_;
        private LearningModelSession postProcessingSession_;

        Classifier currentModel_ = Classifier.SqueezeNet;
        Classifier loadedModel_ = Classifier.NotSet;
        const long BatchSize = 1;
        const long TopK = 10;

        private Dictionary<Classifier, string> modelDictionary_;
        private Dictionary<Classifier, Func<LearningModel>> postProcessorDictionary_;
        private Dictionary<Classifier, Func<LearningModel>> preProcessorDictionary_;

        private static Dictionary<long, string> imagenetLabels_;
        private static Dictionary<long, string> ilsvrc2013Labels_;

        public BatchedInference()
        {
            this.InitializeComponent();
        }

        private void OpenButton_Clicked(object sender, RoutedEventArgs e)
        {
            var file = PickFile();
            if (file != null)
            {

            }
        }

        private StorageFile PickFile()
        {
            FileOpenPicker openPicker = new FileOpenPicker();
            openPicker.ViewMode = PickerViewMode.Thumbnail;
            openPicker.FileTypeFilter.Add(".jpg");
            openPicker.FileTypeFilter.Add(".bmp");
            openPicker.FileTypeFilter.Add(".png");
            openPicker.FileTypeFilter.Add(".jpeg");

            // When running on win32, FileOpenPicker needs to know the top-level hwnd via IInitializeWithWindow::Initialize.
            if (Window.Current == null)
            {
                var picker_unknown = Marshal.GetComInterfaceForObject(openPicker, typeof(IInitializeWithWindow));
                var initializeWithWindowWrapper = (IInitializeWithWindow)Marshal.GetTypedObjectForIUnknown(picker_unknown, typeof(IInitializeWithWindow));
                IntPtr hwnd = GetActiveWindow();
                initializeWithWindowWrapper.Initialize(hwnd);
            }

            return openPicker.PickSingleFileAsync().GetAwaiter().GetResult();
        }

        private static SoftwareBitmap CreateSoftwareBitmapFromStorageFile(StorageFile file)
        {
            var stream = file.OpenAsync(FileAccessMode.Read).GetAwaiter().GetResult();
            var decoder = BitmapDecoder.CreateAsync(stream).GetAwaiter().GetResult();
            return decoder.GetSoftwareBitmapAsync().GetAwaiter().GetResult();
        }

        private void LoadLabelsAndModelPaths()
        {
            if (imagenetLabels_ == null)
            {
                imagenetLabels_ = LoadLabels("ms-appx:///InputData/sysnet.txt");
            }

            if (ilsvrc2013Labels_ == null)
            {
                ilsvrc2013Labels_ = LoadLabels("ms-appx:///InputData/ilsvrc2013.txt");
            }

            if (modelDictionary_ == null)
            {
                modelDictionary_ = new Dictionary<Classifier, string>
                {
                    { Classifier.SqueezeNet, "ms-appx:///Models/squeezenet1.1-7.onnx" },

                };
            }

            if (postProcessorDictionary_ == null)
            {
                postProcessorDictionary_ = new Dictionary<Classifier, Func<LearningModel>>{
                    { Classifier.SqueezeNet, () => TensorizationModels.SoftMaxThenTopK(TopK) },
                };
            }

        }

        private void InitializeWindowsMachineLearning(Classifier model, int batchSizeOverride=-1)
        {
            if (currentModel_ != loadedModel_)
            {
                var modelPath = modelDictionary_[model];
                inferenceSession_ = CreateLearningModelSession(modelPath, batchSizeOverride);
                preProcessingSession_ = null;
                Func<LearningModel> postProcessor = () => TensorizationModels.SoftMaxThenTopK(TopK);
                postProcessingSession_ = CreateLearningModelSession(postProcessor(), batchSizeOverride);

                if (currentModel_ == Classifier.RCNN_ILSVRC13)
                {
                    labels_ = ilsvrc2013Labels_;
                }
                else
                {
                    labels_ = imagenetLabels_;
                }

                loadedModel_ = currentModel_;
            }
        }

        private void ResetModels()
        {
            currentModel_ = Classifier.SqueezeNet;
            loadedModel_ = Classifier.NotSet;
        }

        private LearningModelSession CreateLearningModelSession(string modelPath, int batchSizeOverride=-1)
        {
            var model = CreateLearningModel(modelPath);
            var session = CreateLearningModelSession(model, batchSizeOverride);
            return session;
        }

        private LearningModelSession CreateLearningModelSession(LearningModel model, int batchSizeOverride=-1)
        {
            var kind =
                (DeviceComboBox.SelectedIndex == 0) ?
                    LearningModelDeviceKind.Cpu :
                    LearningModelDeviceKind.DirectXHighPerformance;
            var device = new LearningModelDevice(kind);
            var options = new LearningModelSessionOptions()
            {
                CloseModelOnSessionCreation = true // Close the model to prevent extra memory usage
            };
            if (batchSizeOverride > 0)
            {
                options.BatchSizeOverride = (uint) batchSizeOverride;
            }
            var session = new LearningModelSession(model, device, options);
            return session;
        }

        private static LearningModel CreateLearningModel(string modelPath)
        {
            var uri = new Uri(modelPath);
            var file = StorageFile.GetFileFromApplicationUriAsync(uri).GetAwaiter().GetResult();
            return LearningModel.LoadFromStorageFileAsync(file).GetAwaiter().GetResult();
        }

        private void StartInference(object sender, RoutedEventArgs e)
        {
            var birdFile = StorageFile.GetFileFromApplicationUriAsync(new Uri("ms-appx:///InputData/hummingbird.jpg")).GetAwaiter().GetResult();
            var catFile = StorageFile.GetFileFromApplicationUriAsync(new Uri("ms-appx:///InputData/kitten.png")).GetAwaiter().GetResult();
            var fishFile = StorageFile.GetFileFromApplicationUriAsync(new Uri("ms-appx:///InputData/fish.png")).GetAwaiter().GetResult();
            var birdImage = CreateSoftwareBitmapFromStorageFile(birdFile);
            var catImage = CreateSoftwareBitmapFromStorageFile(catFile);
            var fishImage = CreateSoftwareBitmapFromStorageFile(fishFile);
            var images = new List<SoftwareBitmap> { birdImage, catImage, fishImage };
            LoadLabelsAndModelPaths();
            /*            InitializeWindowsMachineLearning(currentModel_);
                        var (individualResults, totalMetricTimes) = Classify(images);
                        RenderInferenceResults(individualResults, totalMetricTimes);*/


            InitializeWindowsMachineLearning(currentModel_, 3);
            ClassifyBatched(images);

            ResetModels();
        }

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

        private (List<InferenceResult>, List<TotalMetricTime>) Classify(List<SoftwareBitmap> images)
        {
            var individualResults = new List<InferenceResult>();
            var totalMetricTimes = new List<TotalMetricTime>();
            float totalPreprocessTime = 0;
            float totalInferenceTime = 0;
            float totalPostprocessTime = 0;

            images.ForEach(delegate (SoftwareBitmap image) {

                long start, stop;
                var input = (object)VideoFrame.CreateWithSoftwareBitmap(image);

                // PreProcess
                start = HighResolutionClock.UtcNow();
                object preProcessedOutput = input;
                if (preProcessingSession_ != null)
                {
                    var preProcessedResults = Evaluate(preProcessingSession_, input);
                    preProcessedOutput = preProcessedResults.Outputs.First().Value;
                    var preProcessedOutputTF = preProcessedOutput as TensorFloat;
                    var shape = preProcessedOutputTF.Shape;
                    System.Diagnostics.Debug.WriteLine("shape = {0}, {1}, {2}, {3}", shape[0], shape[1], shape[2], shape[3]);
                }
                stop = HighResolutionClock.UtcNow();
                var preprocessDuration = HighResolutionClock.DurationInMs(start, stop);

                // Inference
                start = HighResolutionClock.UtcNow();
                var inferenceResults = Evaluate(inferenceSession_, preProcessedOutput);
                var inferenceOutput = inferenceResults.Outputs.First().Value as TensorFloat;
                stop = HighResolutionClock.UtcNow();
                var inferenceDuration = HighResolutionClock.DurationInMs(start, stop);

                // PostProcess
                start = HighResolutionClock.UtcNow();
                var postProcessedOutputs = Evaluate(postProcessingSession_, inferenceOutput);
                var topKValues = postProcessedOutputs.Outputs["TopKValues"] as TensorFloat;
                var topKIndices = postProcessedOutputs.Outputs["TopKIndices"] as TensorInt64Bit;

                // Return results
                var probabilities = topKValues.GetAsVectorView();
                var indices = topKIndices.GetAsVectorView();
                var labels = indices.Select((index) => labels_[index]);
                var most_confident_label = labels.First();
                stop = HighResolutionClock.UtcNow();
                var postProcessDuration = HighResolutionClock.DurationInMs(start, stop);

                var result = new InferenceResult
                {
                    Label = most_confident_label,
                    PreprocessTime = preprocessDuration.ToString(),
                    InferenceTime = inferenceDuration.ToString(),
                    PostprocessTime = postProcessDuration.ToString()
                };

                individualResults.Add(result);
                totalPreprocessTime += preprocessDuration;
                totalInferenceTime += inferenceDuration;
                totalPostprocessTime += postProcessDuration;
            });

            totalMetricTimes.Add(new TotalMetricTime
            {
                Metric = "Total Preprocess Time",
                TotalTime = totalPreprocessTime.ToString()
            });
            totalMetricTimes.Add(new TotalMetricTime
            {
                Metric = "Total Inference Time",
                TotalTime = totalInferenceTime.ToString()
            });
            totalMetricTimes.Add(new TotalMetricTime
            {
                Metric = "Total Postprocess Time",
                TotalTime = totalPostprocessTime.ToString()
            });

            return (individualResults, totalMetricTimes);
        }

        private void ClassifyBatched(List<SoftwareBitmap> images)
        {
            var individualResults = new List<InferenceResult>();
            var totalMetricTimes = new List<TotalMetricTime>();
            float totalPreprocessTime = 0;
            float totalInferenceTime = 0;
            float totalPostprocessTime = 0;

            long start, stop;
            var input = new List<VideoFrame>();
            images.ForEach(delegate (SoftwareBitmap image)
            {
                input.Add(VideoFrame.CreateWithSoftwareBitmap(image));
            });

            // PreProcess
            start = HighResolutionClock.UtcNow();
            var preProcessedOutput = input;
/*            if (preProcessingSession_ != null)
            {
                System.Diagnostics.Debug.WriteLine("Evaluating for preprocessing");
                var preProcessedResults = EvaluateBatched(preProcessingSession_, input);
                preProcessedOutput = preProcessedResults.Outputs.First().Value;
                var preProcessedOutputTF = preProcessedOutput as TensorFloat;
                var shape = preProcessedOutputTF.Shape;
                System.Diagnostics.Debug.WriteLine("shape = {0}, {1}, {2}, {3}", shape[0], shape[1], shape[2], shape[3]);
            }*/
            stop = HighResolutionClock.UtcNow();
            var preprocessDuration = HighResolutionClock.DurationInMs(start, stop);

            // Inference
            start = HighResolutionClock.UtcNow();
            var inferenceResults = EvaluateBatched(inferenceSession_, preProcessedOutput);
            var inferenceOutput = inferenceResults.Outputs.First().Value as TensorFloat;
  
            // PostProcess
            var outputVector = inferenceOutput.GetAsVectorView();
            var outputList = new List<float>(outputVector);
            var batchSize = input.Count;
            System.Diagnostics.Debug.WriteLine("batchSize {0}", batchSize);
            var oneOutputSize = outputList.Count / batchSize;
            System.Diagnostics.Debug.WriteLine("oneOutputSize {0}", oneOutputSize);
            // For each batch find the highest probability along with its label index
            for (int batchId = 0; batchId < batchSize; batchId++)
            {
                float topProbability = 0;
                int topProbabilityIndex = 0;
                for (int i = 0; i < oneOutputSize; i++)
                {
                    var currentProbability = outputList[i + oneOutputSize * batchId];
                    if (currentProbability > topProbability)
                    {
                        topProbability = currentProbability;
                        topProbabilityIndex = i;
                    }
                }

                var topLabel = labels_[topProbabilityIndex];
                System.Diagnostics.Debug.WriteLine("Results for batch {0}", batchId);
                System.Diagnostics.Debug.WriteLine("Top label: {0}, with probability {1}", topLabel, topProbability);

            }

            stop = HighResolutionClock.UtcNow();
            var inferenceDuration = HighResolutionClock.DurationInMs(start, stop);

            // PostProcess
/*            start = HighResolutionClock.UtcNow();
            var postProcessedOutputs = Evaluate(postProcessingSession_, inferenceOutput);
            var topKValues = postProcessedOutputs.Outputs["TopKValues"] as TensorFloat;
            var topKIndices = postProcessedOutputs.Outputs["TopKIndices"] as TensorInt64Bit;

            // Return results
            var probabilities = topKValues.GetAsVectorView();
            var indices = topKIndices.GetAsVectorView();
            var labels = indices.Select((index) => labels_[index]);
            var labels_list = new List<string>(labels);
            labels_list.ForEach(delegate (string label)
            {
                System.Diagnostics.Debug.WriteLine(label);
            });*/
            /*var most_confident_label = labels.First();*/
            stop = HighResolutionClock.UtcNow();
            var postProcessDuration = HighResolutionClock.DurationInMs(start, stop);
/*
            var result = new InferenceResult
            {
                Label = most_confident_label,
                PreprocessTime = preprocessDuration.ToString(),
                InferenceTime = inferenceDuration.ToString(),
                PostprocessTime = postProcessDuration.ToString()
            };

            individualResults.Add(result);
            totalPreprocessTime += preprocessDuration;
            totalInferenceTime += inferenceDuration;
            totalPostprocessTime += postProcessDuration;

            totalMetricTimes.Add(new TotalMetricTime
            {
                Metric = "Total Preprocess Time",
                TotalTime = totalPreprocessTime.ToString()
            });
            totalMetricTimes.Add(new TotalMetricTime
            {
                Metric = "Total Inference Time",
                TotalTime = totalInferenceTime.ToString()
            });
            totalMetricTimes.Add(new TotalMetricTime
            {
                Metric = "Total Postprocess Time",
                TotalTime = totalPostprocessTime.ToString()
            });*/

            /*return (individualResults, totalMetricTimes);*/
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
            binding.Bind(outputName, output);

            // Evaluate
            return session.Evaluate(binding, "");
        }

        private static LearningModelEvaluationResult EvaluateBatched(LearningModelSession session, List<VideoFrame> input)
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
            /*binding.Bind(outputName, output);*/

            // Evaluate
            return session.Evaluate(binding, "");
        }

        private void DeviceComboBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
        }

        private void RenderInferenceResults(List<InferenceResult> individualResults,
            List<TotalMetricTime> totalMetricTimes)
        {
            var individualResultsHeader = new InferenceResult {
                Label = "Label",
                PreprocessTime = "Preprocess Time (ms)",
                InferenceTime = "Inference Time (ms)",
                PostprocessTime = "Postprocess Time (ms)"
            };
            individualResults.Insert(0, individualResultsHeader);
            var totalMetricTimesHeader = new TotalMetricTime
            {
                Metric = "Metric",
                TotalTime = "TotalTime"
            };
            totalMetricTimes.Insert(0, totalMetricTimesHeader);

            InferenceResults.ItemsSource = individualResults;
            InferenceResults.SelectedIndex = 0;
            TotalInferenceResults.ItemsSource = totalMetricTimes;
            TotalInferenceResults.SelectedIndex = 0;
        }

    }
}
