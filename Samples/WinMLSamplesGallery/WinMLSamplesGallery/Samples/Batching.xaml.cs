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
        private LearningModelSession nonBatchingSession_;
        private LearningModelSession BatchingSession_;
        float avgNonBatchedDuration = 0;
        float avgBatchDuration = 0;

        public Batching()
        {
            this.InitializeComponent();
        }

        async private void StartInference(object sender, RoutedEventArgs e)
        {
            ShowEvalUI();
            ResetEvalMetrics();

            var inputImages = GetInputImages();
            CreateSessions(inputImages.Count);

            UpdateEvalText(false);
            await Classify(inputImages);

            UpdateEvalText(true);
            await ClassifyBatched(inputImages);

            GenerateEvalResultAndUI();
        }

        private void ShowEvalUI()
        {
            StartInferenceBtn.IsEnabled = false;
            EvalResults.Visibility = Visibility.Collapsed;
            LoadingContainer.Visibility = Visibility.Visible;
        }

        private void ResetEvalMetrics()
        {
            avgNonBatchedDuration = 0;
            avgBatchDuration = 0;
        }

        private List<VideoFrame> GetInputImages()
        {
            var birdFile = StorageFile.GetFileFromApplicationUriAsync(new Uri("ms-appx:///InputData/hummingbird.jpg")).GetAwaiter().GetResult();
            var catFile = StorageFile.GetFileFromApplicationUriAsync(new Uri("ms-appx:///InputData/kitten.png")).GetAwaiter().GetResult();
            var birdImage = CreateSoftwareBitmapFromStorageFile(birdFile);
            var catImage = CreateSoftwareBitmapFromStorageFile(catFile);
            var inputImages = new List<VideoFrame>();
            for (int i = 0; i < 25; i++)
            {
                inputImages.Add(VideoFrame.CreateWithSoftwareBitmap(birdImage));
                inputImages.Add(VideoFrame.CreateWithSoftwareBitmap(catImage));
            }
            return inputImages;
        }

        private SoftwareBitmap CreateSoftwareBitmapFromStorageFile(StorageFile file)
        {
            var stream = file.OpenAsync(FileAccessMode.Read).GetAwaiter().GetResult();
            var decoder = BitmapDecoder.CreateAsync(stream).GetAwaiter().GetResult();
            return decoder.GetSoftwareBitmapAsync().GetAwaiter().GetResult();
        }

        private void UpdateEvalText(bool isBatchingEval)
        {
            if (isBatchingEval)
                EvalText.Text = "Inferencing Batched Inputs:";
            else
                EvalText.Text = "Inferencing Non-Batched Inputs:";
        }

        private void CreateSessions(int batchSizeOverride)
        {
            var modelPath = "ms-appx:///Models/squeezenet1.1-7.onnx";
            nonBatchingSession_ = CreateLearningModelSession(modelPath);
            BatchingSession_ = CreateLearningModelSession(modelPath, batchSizeOverride);
        }

        private LearningModelSession CreateLearningModelSession(string modelPath, int batchSizeOverride=-1)
        {
            var model = CreateLearningModel(modelPath);
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

        async private System.Threading.Tasks.Task Classify(List<VideoFrame> inputImages)
        {
            float totalEvalDurations = 0;
            for (int i = 0; i < 100; i++)
            {
                UpdateEvalProgressUI(i);
                float evalDuration = await System.Threading.Tasks.Task.Run(() => Evaluate(nonBatchingSession_, inputImages));
                totalEvalDurations += evalDuration;
            }
            avgNonBatchedDuration = totalEvalDurations / 100;
        }

        private static float Evaluate(LearningModelSession session, List<VideoFrame> input)
        {
            string inputName = session.Model.InputFeatures[0].Name;
            float totalDuration = 0;
            for (int j = 0; j < input.Count; j++)
            {
                var binding = new LearningModelBinding(session);
                var start = HighResolutionClock.UtcNow();
                binding.Bind(inputName, input[j]);
                session.Evaluate(binding, "");
                var stop = HighResolutionClock.UtcNow();
                var duration = HighResolutionClock.DurationInMs(start, stop);
                totalDuration += duration;
            }
            return totalDuration;
        }

        async private System.Threading.Tasks.Task ClassifyBatched(List<VideoFrame> inputImages)
        {
            float totalEvalDurations = 0;
            for (int i = 0; i < 100; i++)
            {
                UpdateEvalProgressUI(i);
                float evalDuration = await System.Threading.Tasks.Task.Run(() => EvaluateBatched(BatchingSession_, inputImages));
                totalEvalDurations += evalDuration;
            }
            avgBatchDuration = totalEvalDurations / 100;
        }

        private static float EvaluateBatched(LearningModelSession session, List<VideoFrame> input)
        {
            // Bind
            var binding = new LearningModelBinding(session);
            string inputName = session.Model.InputFeatures[0].Name;
            var bindStart = HighResolutionClock.UtcNow();
            binding.Bind(inputName, input);
            var bindStop = HighResolutionClock.UtcNow();
            var bindDuration = HighResolutionClock.DurationInMs(bindStart, bindStop);
            // Evaluate
            var evalStart = HighResolutionClock.UtcNow();
            session.Evaluate(binding, "");
            var evalStop = HighResolutionClock.UtcNow();
            var evalDuration = HighResolutionClock.DurationInMs(evalStart, evalStop);
            float totalDuration = bindDuration + evalDuration;
            return totalDuration;
        }

        private void UpdateEvalProgressUI(int attemptNumber)
        {
            EvalProgressText.Text = "Attempt " + attemptNumber.ToString() + "/100";
            EvalProgressBar.Value = attemptNumber + 1;
        }

        private void GenerateEvalResultAndUI()
        {
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
        }
    }
}
