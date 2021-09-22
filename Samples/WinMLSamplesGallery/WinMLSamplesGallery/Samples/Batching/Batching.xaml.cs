using System;
using System.Collections.Generic;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Windows.Storage;
using Windows.Graphics.Imaging;
using Windows.Media;
using Microsoft.AI.MachineLearning;

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
        static bool navigatingAwayFromPage = false;
        const int numInputImages = 50;

        public Batching()
        {
            this.InitializeComponent();
            // Ensure static variable is always false on page initialization
            navigatingAwayFromPage = false;
        }

        async private void StartInference(object sender, RoutedEventArgs e)
        {
            ShowEvalUI();
            ResetEvalMetrics();

            var inputImages = GetInputImages();
            int batchSize = GetBatchSizeFromBatchSizeComboBox();
            CreateSessions(batchSize);

            UpdateEvalText(false);
            await Classify(inputImages);

            UpdateEvalText(true);
            await ClassifyBatched(inputImages, batchSize);

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

        // Test input consists of 50 images (25 bird and 25 cat)
        private List<VideoFrame> GetInputImages()
        {
            var birdFile = StorageFile.GetFileFromApplicationUriAsync(new Uri("ms-appx:///InputData/hummingbird.jpg")).GetAwaiter().GetResult();
            var catFile = StorageFile.GetFileFromApplicationUriAsync(new Uri("ms-appx:///InputData/kitten.png")).GetAwaiter().GetResult();
            var birdImage = CreateSoftwareBitmapFromStorageFile(birdFile);
            var catImage = CreateSoftwareBitmapFromStorageFile(catFile);
            var inputImages = new List<VideoFrame>();
            for (int i = 0; i < numInputImages / 2; i++)
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
            var deviceKind = DeviceComboBox.GetDeviceKind();
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
                if (navigatingAwayFromPage)
                    break;
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
                if (navigatingAwayFromPage)
                    break;
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

        async private System.Threading.Tasks.Task ClassifyBatched(List<VideoFrame> inputImages, int batchSize)
        {
            float totalEvalDurations = 0;
            for (int i = 0; i < 100; i++)
            {
                if (navigatingAwayFromPage)
                    break;
                UpdateEvalProgressUI(i);
                float evalDuration = await System.Threading.Tasks.Task.Run(() => EvaluateBatched(BatchingSession_, inputImages, batchSize));
                totalEvalDurations += evalDuration;
            }
            avgBatchDuration = totalEvalDurations / 100;
        }

        private static float EvaluateBatched(LearningModelSession session, List<VideoFrame> input, int batchSize)
        {
            // Evaluate
            int numBatches = input.Count / batchSize;
            string inputName = session.Model.InputFeatures[0].Name;
            float totalDuration = 0;
            for (int i = 0; i < numBatches; i++)
            {
                if (navigatingAwayFromPage)
                    break;
                var binding = new LearningModelBinding(session);
                List<VideoFrame> batch = input.GetRange(batchSize * i, batchSize);
                var start = HighResolutionClock.UtcNow();
                binding.Bind(inputName, batch);
                session.Evaluate(binding, "");
                var stop = HighResolutionClock.UtcNow();
                var duration = HighResolutionClock.DurationInMs(start, stop);
                totalDuration += duration;
            }
            return totalDuration;
        }

        private int GetBatchSizeFromBatchSizeComboBox()
        {
            int batchSize;
            if (BatchSizeComboBox.SelectedIndex == 0)
                batchSize = 5;
            else if (BatchSizeComboBox.SelectedIndex == 1)
                batchSize = 10;
            else if (BatchSizeComboBox.SelectedIndex == 2)
                batchSize = 25;
            else
                batchSize = 50;
            return batchSize;
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

        public void StopAllEvents()
        {
            navigatingAwayFromPage = true;
        }
    }
}
