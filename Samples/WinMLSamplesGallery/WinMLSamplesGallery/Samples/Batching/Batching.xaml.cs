using Microsoft.AI.MachineLearning;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using System;
using System.Collections.Generic;
using System.Threading.Tasks;
using Windows.Graphics.Imaging;
using Windows.Media;
using Windows.Storage;

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
        const int numInputImages = 50;
        const int numEvalIterations = 100;

        private LearningModelSession nonBatchingSession_;
        private LearningModelSession batchingSession_;

        float avgNonBatchedDuration_ = 0;
        float avgBatchDuration_ = 0;

        // Marked volatile since it's updated across threads
        static volatile bool navigatingAwayFromPage = false;


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

            var inputImages = await GetInputImages();
            int batchSize = GetBatchSizeFromBatchSizeSlider();
            await CreateSessions(batchSize);

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
            avgNonBatchedDuration_ = 0;
            avgBatchDuration_ = 0;
        }

        // Test input consists of 50 images (25 bird and 25 cat)
        private async Task<List<VideoFrame>> GetInputImages()
        {
            var birdFile = await StorageFile.GetFileFromApplicationUriAsync(new Uri("ms-appx:///InputData/hummingbird.jpg"));
            var catFile = await StorageFile .GetFileFromApplicationUriAsync(new Uri("ms-appx:///InputData/kitten.png"));
            var birdImage = await CreateSoftwareBitmapFromStorageFile(birdFile);
            var catImage = await CreateSoftwareBitmapFromStorageFile(catFile);
            var inputImages = new List<VideoFrame>();
            for (int i = 0; i < numInputImages / 2; i++)
            {
                inputImages.Add(VideoFrame.CreateWithSoftwareBitmap(birdImage));
                inputImages.Add(VideoFrame.CreateWithSoftwareBitmap(catImage));
            }
            return inputImages;
        }

        private async Task<SoftwareBitmap> CreateSoftwareBitmapFromStorageFile(StorageFile file)
        {
            var stream = await file.OpenAsync(FileAccessMode.Read);
            var decoder = await BitmapDecoder.CreateAsync(stream);
            var bitmap = await decoder.GetSoftwareBitmapAsync();
            return bitmap;
        }

        private void UpdateEvalText(bool isBatchingEval)
        {
            if (isBatchingEval)
                EvalText.Text = "Inferencing Batched Inputs:";
            else
                EvalText.Text = "Inferencing Non-Batched Inputs:";
        }

        private async Task CreateSessions(int batchSizeOverride)
        {
            var modelPath = "ms-appx:///Models/squeezenet1.1-7.onnx";
            nonBatchingSession_ = await CreateLearningModelSession(modelPath);
            batchingSession_ = await CreateLearningModelSession(modelPath, batchSizeOverride);
        }

        private async Task<LearningModelSession> CreateLearningModelSession(string modelPath, int batchSizeOverride=-1)
        {
            var model = await CreateLearningModel(modelPath);
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

        private static async Task<LearningModel> CreateLearningModel(string modelPath)
        {
            var uri = new Uri(modelPath);
            var file = await StorageFile.GetFileFromApplicationUriAsync(uri);
            var model = await LearningModel.LoadFromStorageFileAsync(file);
            return model;
        }

        async private Task Classify(List<VideoFrame> inputImages)
        {
            float totalEvalDurations = 0;
            for (int i = 0; i < numEvalIterations; i++)
            {
                if (navigatingAwayFromPage)
                    break;
                UpdateEvalProgressUI(i);
                float evalDuration = await Task.Run(() => Evaluate(nonBatchingSession_, inputImages));
                totalEvalDurations += evalDuration;
            }
            avgNonBatchedDuration_ = totalEvalDurations / numEvalIterations;
        }

        private static float Evaluate(LearningModelSession session, List<VideoFrame> input)
        {
            string inputName = session.Model.InputFeatures[0].Name;
            float totalDuration = 0;
            var binding = new LearningModelBinding(session);
            for (int j = 0; j < input.Count; j++)
            {
                if (navigatingAwayFromPage)
                    break;
                var start = HighResolutionClock.UtcNow();
                binding.Bind(inputName, input[j]);
                session.Evaluate(binding, "");
                var stop = HighResolutionClock.UtcNow();
                var duration = HighResolutionClock.DurationInMs(start, stop);
                totalDuration += duration;
            }
            return totalDuration;
        }

        async private Task ClassifyBatched(List<VideoFrame> inputImages, int batchSize)
        {
            float totalEvalDurations = 0;
            for (int i = 0; i < numEvalIterations; i++)
            {
                if (navigatingAwayFromPage)
                    break;
                UpdateEvalProgressUI(i);
                float evalDuration = await Task.Run(() => EvaluateBatched(batchingSession_, inputImages, batchSize));
                totalEvalDurations += evalDuration;
            }
            avgBatchDuration_ = totalEvalDurations / numEvalIterations;
        }

        private static float EvaluateBatched(LearningModelSession session, List<VideoFrame> input, int batchSize)
        {
            int numBatches = input.Count / batchSize;
            string inputName = session.Model.InputFeatures[0].Name;
            float totalDuration = 0;
            var binding = new LearningModelBinding(session);
            for (int i = 0; i < numBatches; i++)
            {
                if (navigatingAwayFromPage)
                    break;
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

        private int GetBatchSizeFromBatchSizeSlider()
        {
            return int.Parse(BatchSizeSlider.Value.ToString());
        }

        private void UpdateEvalProgressUI(int attemptNumber)
        {
            EvalProgressText.Text = "Attempt " + attemptNumber.ToString() + "/" + numEvalIterations.ToString();
            EvalProgressBar.Value = attemptNumber + 1;
        }

        private void GenerateEvalResultAndUI()
        {
            float ratio = (1 - (avgBatchDuration_ / avgNonBatchedDuration_)) * 100;
            var evalResult = new EvalResult
            {
                nonBatchedAvgTime = avgNonBatchedDuration_.ToString("0.00"),
                batchedAvgTime = avgBatchDuration_.ToString("0.00"),
                timeRatio = ratio.ToString("0.0")
            };
            List<EvalResult> results = new List<EvalResult>();
            results.Insert(0, evalResult);
            LoadingContainer.Visibility = Visibility.Collapsed;
            EvalResults.Visibility = Visibility.Visible;
            StartInferenceBtn.IsEnabled = true;
            EvalResults.ItemsSource = results;
        }

        private void UpdateBatchSizeText(object sender, RoutedEventArgs e)
        {
            BatchSizeText.Text = "Batch Size: " + BatchSizeSlider.Value.ToString();
        }

        public void StopAllEvents()
        {
            navigatingAwayFromPage = true;
        }
    }
}
