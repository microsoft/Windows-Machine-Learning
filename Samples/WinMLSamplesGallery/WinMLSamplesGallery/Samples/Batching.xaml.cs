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

    public sealed class EvalResult
    {
        public string nonBatchedAvgTime { get; set; }
        public string batchedAvgTime { get; set; }
        public string timeRatio { get; set; }
    }


    public sealed class TotalMetricTime
    {
        public string Metric { get; set; }
        public string TotalTime { get; set; }
    }

    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class Batching : Page
    {
        [DllImport("user32.dll", ExactSpelling = true, CharSet = CharSet.Auto, PreserveSig = true, SetLastError = false)]
        public static extern IntPtr GetActiveWindow();

        private static Dictionary<long, string> labels_;
        private LearningModelSession nonBatchingSession_;
        private LearningModelSession BatchingSession_;
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

        public Batching()
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
                nonBatchingSession_ = CreateLearningModelSession(modelPath);
                BatchingSession_ = CreateLearningModelSession(modelPath, batchSizeOverride);
                preProcessingSession_ = null;
/*                Func<LearningModel> postProcessor = () => TensorizationModels.SoftMaxThenTopK(TopK);
                postProcessingSession_ = CreateLearningModelSession(postProcessor(), batchSizeOverride);*/

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
/*            var kind =
                (DeviceComboBox.SelectedIndex == 0) ?
                    LearningModelDeviceKind.Cpu :
                    LearningModelDeviceKind.DirectXHighPerformance;*/
            var device = new LearningModelDevice(LearningModelDeviceKind.Cpu);
            var options = new LearningModelSessionOptions()
            {
                CloseModelOnSessionCreation = true // Close the model to prevent extra memory usage
            };
            if (batchSizeOverride > 0)
            {
                options.BatchSizeOverride = (uint)batchSizeOverride;
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
            System.Diagnostics.Debug.WriteLine("Visibility before {0}", RunningText.Visibility);
            RunningText.Visibility = Visibility.Visible;
            System.Diagnostics.Debug.WriteLine("Visibility after {0}", RunningText.Visibility);

            var birdFile = StorageFile.GetFileFromApplicationUriAsync(new Uri("ms-appx:///InputData/hummingbird.jpg")).GetAwaiter().GetResult();
            var catFile = StorageFile.GetFileFromApplicationUriAsync(new Uri("ms-appx:///InputData/kitten.png")).GetAwaiter().GetResult();
            var fishFile = StorageFile.GetFileFromApplicationUriAsync(new Uri("ms-appx:///InputData/fish.png")).GetAwaiter().GetResult();
            var birdImage = CreateSoftwareBitmapFromStorageFile(birdFile);
            var catImage = CreateSoftwareBitmapFromStorageFile(catFile);
            var fishImage = CreateSoftwareBitmapFromStorageFile(fishFile);
            var images = new List<SoftwareBitmap>();
            for (int i = 0; i < 20; i++)
            {
                images.Add(birdImage);
                images.Add(catImage);
                images.Add(fishImage);
            }

            LoadLabelsAndModelPaths();
            InitializeWindowsMachineLearning(currentModel_, images.Count);

            /*var evalResult = new System.Threading.Thread(new System.Threading.ThreadStart(DoClassifications(images)));*/
            EvalResult evalResult = null;
            /*System.Threading.Thread t = new System.Threading.Thread(() => DoClassifications(images));*/
            System.Threading.Thread t = new System.Threading.Thread(() => {
                float avgNonBatchedDuration = Classify(images);
                float avgBatchDuration = ClassifyBatched(images);
                float ratio = 1 / (avgBatchDuration / avgNonBatchedDuration);
                System.Diagnostics.Debug.WriteLine("Average Non-Batch Duration {0}", avgNonBatchedDuration);
                System.Diagnostics.Debug.WriteLine("Average Batch Duration {0}", avgBatchDuration);
                evalResult = new EvalResult
                {
                    nonBatchedAvgTime = avgNonBatchedDuration.ToString("0.00"),
                    batchedAvgTime = avgBatchDuration.ToString("0.00"),
                    timeRatio = ratio.ToString("0.0")
                };
            });

            t.Start();
            t.Join();
  /*          var evalResult = new EvalResult
            {
                nonBatchedAvgTime = "10",
                batchedAvgTime = "20"
            };*/
            List<EvalResult> results = new List<EvalResult>();
            results.Insert(0, evalResult);
            RunningText.Visibility = Visibility.Visible;
            Results.ItemsSource = results;
            /*            RenderInferenceResults(individualResults, totalMetricTimes);*/

            ResetModels();
        }

        private void DoClassifications(List<SoftwareBitmap> images)
        {
            float avgNonBatchedDuration = Classify(images);
            float avgBatchDuration = ClassifyBatched(images);
            float ratio = 1 / (avgBatchDuration / avgNonBatchedDuration);
            System.Diagnostics.Debug.WriteLine("Average Non-Batch Duration {0}", avgNonBatchedDuration);
            System.Diagnostics.Debug.WriteLine("Average Batch Duration {0}", avgBatchDuration);
            var evalResult = new EvalResult
            {
                nonBatchedAvgTime = avgNonBatchedDuration.ToString("0.00"),
                batchedAvgTime = avgBatchDuration.ToString("0.00"),
                timeRatio = ratio.ToString("0.0")
            };
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

        private float Classify(List<SoftwareBitmap> images)
        {
            var individualResults = new List<InferenceResult>();
            var totalMetricTimes = new List<TotalMetricTime>();
            float totalPreprocessTime = 0;
            float totalInferenceTime = 0;
            float totalPostprocessTime = 0;

            var input = new List<VideoFrame>();
            images.ForEach(delegate (SoftwareBitmap image)
            {
                input.Add(VideoFrame.CreateWithSoftwareBitmap(image));
            });
            return Evaluate(nonBatchingSession_, input);
        }

        private float ClassifyBatched(List<SoftwareBitmap> images)
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

            // Inference

            return EvaluateBatched(BatchingSession_, input);

/*            var inferenceResults = EvaluateBatched(inferenceSession_, preProcessedOutput);*/
/*            var inferenceOutput = inferenceResults.Outputs.First().Value as TensorFloat;*/
  
            // PostProcess
/*            var outputVector = inferenceOutput.GetAsVectorView();
            var outputList = new List<float>(outputVector);
            var batchSize = input.Count;
            System.Diagnostics.Debug.WriteLine("batchSize {0}", batchSize);
            var oneOutputSize = outputList.Count / batchSize;
            System.Diagnostics.Debug.WriteLine("oneOutputSize {0}", oneOutputSize);*/
            // For each batch find the highest probability along with its label index
/*            for (int batchId = 0; batchId < batchSize; batchId++)
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
*//*                System.Diagnostics.Debug.WriteLine("Results for batch {0}", batchId);
                System.Diagnostics.Debug.WriteLine("Top label: {0}, with probability {1}", topLabel, topProbability);*//*

            }*/

        }

        private static float Evaluate(LearningModelSession session, List<VideoFrame> input)
        {
            // Create the binding

            // Create an emoty output, that will keep the output resources on the GPU
            // It will be chained into a the post processing on the GPU as well
            var output = TensorFloat.Create();

            // Bind inputs and outputs
            // For squeezenet these evaluate to "data", and "squeezenet0_flatten0_reshape0"
            string inputName = session.Model.InputFeatures[0].Name;
            string outputName = session.Model.OutputFeatures[0].Name;
            float totalDuration = 0;
            for (int i = 0; i < 100; i++)
            {
                System.Diagnostics.Debug.WriteLine("Non-Batched Iteration {0}", i);
                for (int j = 0; j < input.Count; j++)
                {
                    var start = HighResolutionClock.UtcNow();
                    var binding = new LearningModelBinding(session);
                    binding.Bind(inputName, input[j]);
                    session.Evaluate(binding, "");
                    var stop = HighResolutionClock.UtcNow();
                    var duration = HighResolutionClock.DurationInMs(start, stop);
                    totalDuration += duration;
                }
            }
            float avgDuration = totalDuration / 100;

            // Evaluate
            return avgDuration;
        }

        private static float EvaluateBatched(LearningModelSession session, List<VideoFrame> input)
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
            var bindStart = HighResolutionClock.UtcNow();
            binding.Bind(inputName, input);
            var bindStop = HighResolutionClock.UtcNow();
            var bindDuration = HighResolutionClock.DurationInMs(bindStart, bindStop);
            /*binding.Bind(outputName, output);*/

            // Evaluate
            float totalDuration = bindDuration;
            for (int i = 0; i < 100; i++)
            {
                System.Diagnostics.Debug.WriteLine("Batched Evaluation {0}", i);
                var start = HighResolutionClock.UtcNow();
                session.Evaluate(binding, "");
                var stop = HighResolutionClock.UtcNow();
                var duration = HighResolutionClock.DurationInMs(start, stop);
                totalDuration += duration; 
            }
            float avgDuration = totalDuration / 100;
            /*            System.Diagnostics.Debug.WriteLine("Avg Evaluation Duration after 100 runs {0}", avgDuration);*/
            return avgDuration;
        }

        private void DeviceComboBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
        }

        private void RenderInferenceResults(List<InferenceResult> individualResults,
            List<TotalMetricTime> totalMetricTimes)
        {
/*            var individualResultsHeader = new InferenceResult
            {
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
            TotalInferenceResults.SelectedIndex = 0;*/
        }

    }
}
