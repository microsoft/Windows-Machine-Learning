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
    public sealed class EvalResult
    {
        public string nonBatchedAvgTime { get; set; }
        public string batchedAvgTime { get; set; }
        public string timeRatio { get; set; }
    }

    public sealed partial class Batching : Page
    {
        [DllImport("user32.dll", ExactSpelling = true, CharSet = CharSet.Auto, PreserveSig = true, SetLastError = false)]
        public static extern IntPtr GetActiveWindow();

        private LearningModelSession nonBatchingSession_;
        private LearningModelSession BatchingSession_;

        Classifier currentModel_ = Classifier.SqueezeNet;
        Classifier loadedModel_ = Classifier.NotSet;
        float avgNonBatchedDuration = 0;
        float avgBatchDuration = 0;

        public Batching()
        {
            this.InitializeComponent();
        }

        private static SoftwareBitmap CreateSoftwareBitmapFromStorageFile(StorageFile file)
        {
            var stream = file.OpenAsync(FileAccessMode.Read).GetAwaiter().GetResult();
            var decoder = BitmapDecoder.CreateAsync(stream).GetAwaiter().GetResult();
            return decoder.GetSoftwareBitmapAsync().GetAwaiter().GetResult();
        }

        private void InitializeWindowsMachineLearning(Classifier model, int batchSizeOverride=-1)
        {
            if (currentModel_ != loadedModel_)
            {
                var modelPath = "ms-appx:///Models/squeezenet1.1-7.onnx";
                nonBatchingSession_ = CreateLearningModelSession(modelPath);
                BatchingSession_ = CreateLearningModelSession(modelPath, batchSizeOverride);
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
            var deviceKind =
                (DeviceComboBox.SelectedIndex == 0) ?
                    LearningModelDeviceKind.Cpu :
                    LearningModelDeviceKind.DirectXHighPerformance;
            var device = new LearningModelDevice(deviceKind);
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

        async private void StartInference(object sender, RoutedEventArgs e)
        {
            StartInferenceBtn.IsEnabled = false;
            EvalResults.Visibility = Visibility.Collapsed;
            LoadingContainer.Visibility = Visibility.Visible;
            avgNonBatchedDuration = 0;
            avgBatchDuration = 0;

            var birdFile = StorageFile.GetFileFromApplicationUriAsync(new Uri("ms-appx:///InputData/hummingbird.jpg")).GetAwaiter().GetResult();
            var catFile = StorageFile.GetFileFromApplicationUriAsync(new Uri("ms-appx:///InputData/kitten.png")).GetAwaiter().GetResult();
            var birdImage = CreateSoftwareBitmapFromStorageFile(birdFile);
            var catImage = CreateSoftwareBitmapFromStorageFile(catFile);
            var images = new List<SoftwareBitmap>();
            for (int i = 0; i < 25; i++)
            {
                images.Add(birdImage);
                images.Add(catImage);
            }

            InitializeWindowsMachineLearning(currentModel_, images.Count);
            EvalText.Text = "Inferencing Non-Batched Inputs:";
            await Classify(images);
            EvalText.Text = "Inferencing Batched Inputs:";
            await ClassifyBatched(images);
            float ratio = (1 - (avgBatchDuration / avgNonBatchedDuration)) * 100;
            var evalResult = new EvalResult
            {
                nonBatchedAvgTime = avgNonBatchedDuration.ToString("0.00"),
                batchedAvgTime = avgBatchDuration.ToString("0.00"),
                timeRatio = ratio.ToString("0.0")
            };
            List<EvalResult> results = new List<EvalResult>();
            results.Insert(0, evalResult);
            LoadingContainer.Visibility = Visibility.Collapsed;
            EvalResults.Visibility = Visibility.Visible;
            StartInferenceBtn.IsEnabled = true;
            EvalResults.ItemsSource = results;
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
        async private System.Threading.Tasks.Task Classify(List<SoftwareBitmap> images)
        {
            float totalPreprocessTime = 0;
            float totalInferenceTime = 0;
            float totalPostprocessTime = 0;

            var input = new List<VideoFrame>();
            images.ForEach(delegate (SoftwareBitmap image)
            {
                input.Add(VideoFrame.CreateWithSoftwareBitmap(image));
            });
            float totalEvalDurations = 0;
            for (int i = 0; i < 100; i++)
            {
                EvalProgressText.Text = "Attempt " + i.ToString() + "/100";
                EvalProgressBar.Value = i + 1;
                float evalDuration = await System.Threading.Tasks.Task.Run(() => Evaluate(nonBatchingSession_, input));
                totalEvalDurations += evalDuration;
            }
            avgNonBatchedDuration = totalEvalDurations / 100;
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
            return totalDuration;
        }

        async private System.Threading.Tasks.Task ClassifyBatched(List<SoftwareBitmap> images)
        {
            float totalPreprocessTime = 0;
            float totalInferenceTime = 0;
            float totalPostprocessTime = 0;

            long start, stop;
            var input = new List<VideoFrame>();
            images.ForEach(delegate (SoftwareBitmap image)
            {
                input.Add(VideoFrame.CreateWithSoftwareBitmap(image));
            });
            float totalEvalDurations = 0;
            for (int i = 0; i < 100; i++)
            {
                EvalProgressText.Text = "Attempt " + i.ToString() + "/100";
                EvalProgressBar.Value = i + 1;
                float evalDuration = await System.Threading.Tasks.Task.Run(() => EvaluateBatched(BatchingSession_, input));
                totalEvalDurations += evalDuration;
            }
            avgBatchDuration = totalEvalDurations / 100;
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
            var start = HighResolutionClock.UtcNow();
            session.Evaluate(binding, "");
            var stop = HighResolutionClock.UtcNow();
            var duration = HighResolutionClock.DurationInMs(start, stop);
            return duration;
        }
    }
}
