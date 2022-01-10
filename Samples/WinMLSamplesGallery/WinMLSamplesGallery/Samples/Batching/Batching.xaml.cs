using Microsoft.AI.MachineLearning;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using System;
using System.Collections.Generic;
using System.IO;
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
        const int NumInputImages = 50;
        const int NumEvalIterations = 100;

        private LearningModel _model = null;
        private LearningModelSession _nonBatchingSession = null;
        private LearningModelSession _batchingSession = null;

        float _avgNonBatchedDuration = 0;
        float _avgBatchDuration = 0;

        // Marked volatile since it's updated across threads
        static volatile bool navigatingAwayFromPage = false;


        public Batching()
        {
            this.InitializeComponent();
            // Ensure static variable is always false on page initialization
            navigatingAwayFromPage = false;

            // Load the model
            var modelName = "squeezenet1.1-7-batched.onnx";
            var modelPath = Path.Join(Windows.ApplicationModel.Package.Current.InstalledLocation.Path, "Models", modelName);
            _model = LearningModel.LoadFromFilePath(modelPath);
        }

        async private void StartInference(object sender, RoutedEventArgs e)
        {
            ShowStatus();
            ResetMetrics();

            var inputImages = await GetInputImages();
            int batchSize = GetBatchSizeFromBatchSizeSlider();

            _nonBatchingSession = await CreateLearningModelSession(_model);
            _batchingSession = await CreateLearningModelSession(_model, batchSize);

            UpdateStatus(false);
            await Classify(inputImages);

            UpdateStatus(true);
            await ClassifyBatched(inputImages, batchSize);

            ShowUI();
        }

        private void ShowStatus()
        {
            StartInferenceBtn.IsEnabled = false;
            BatchSizeSlider.IsEnabled = false;
            DeviceComboBox.IsEnabled = false;
            EvalResults.Visibility = Visibility.Collapsed;
            LoadingContainer.Visibility = Visibility.Visible;
        }

        private void ResetMetrics()
        {
            _avgNonBatchedDuration = 0;
            _avgBatchDuration = 0;
        }

        // Test input consists of 50 images (25 bird and 25 cat)
        private async Task<List<VideoFrame>> GetInputImages()
        {
            var birdFile = await StorageFile.GetFileFromApplicationUriAsync(new Uri("ms-appx:///InputData/hummingbird.jpg"));
            var catFile = await StorageFile .GetFileFromApplicationUriAsync(new Uri("ms-appx:///InputData/kitten.png"));
            var birdImage = await CreateSoftwareBitmapFromStorageFile(birdFile);
            var catImage = await CreateSoftwareBitmapFromStorageFile(catFile);
            var inputImages = new List<VideoFrame>();
            for (int i = 0; i < NumInputImages / 2; i++)
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

        private void UpdateStatus(bool isBatchingEval)
        {
            if (isBatchingEval)
            {
                EvalText.Text = "Inferencing Batched Inputs:";
            }
            else
            {
                EvalText.Text = "Inferencing Non-Batched Inputs:";
            }
        }

        private async Task<LearningModelSession> CreateLearningModelSession(LearningModel model, int batchSizeOverride=-1)
        {
            var deviceKind = DeviceComboBox.GetDeviceKind();
            var device = new LearningModelDevice(deviceKind);
            var options = new LearningModelSessionOptions();
            if (batchSizeOverride > 0)
            {
                options.BatchSizeOverride = (uint)batchSizeOverride;
            }
            var session = new LearningModelSession(model, device, options);
            return session;
        }

        async private Task Classify(List<VideoFrame> inputImages)
        {
            float totalEvalDurations = 0;
            for (int i = 0; i < NumEvalIterations; i++)
            {
                if (navigatingAwayFromPage)
                {
                    break;
                }

                UpdateProgress(i);
                float evalDuration = await Task.Run(() => Evaluate(_nonBatchingSession, inputImages));
                totalEvalDurations += evalDuration;
            }
            _avgNonBatchedDuration = totalEvalDurations / NumEvalIterations;
        }

        private static float Evaluate(LearningModelSession session, List<VideoFrame> input)
        {
            string inputName = session.Model.InputFeatures[0].Name;
            float totalDuration = 0;
            var binding = new LearningModelBinding(session);
            for (int j = 0; j < input.Count; j++)
            {
                if (navigatingAwayFromPage)
                {
                    break;
                }

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
            for (int i = 0; i < NumEvalIterations; i++)
            {
                if (navigatingAwayFromPage)
                    break;
                UpdateProgress(i);
                float evalDuration = await Task.Run(() => EvaluateBatched(_batchingSession, inputImages, batchSize));
                totalEvalDurations += evalDuration;
            }
            _avgBatchDuration = totalEvalDurations / NumEvalIterations;
        }

        private static float EvaluateBatched(LearningModelSession session, List<VideoFrame> input, int batchSize)
        {
            int numBatches = (int) Math.Ceiling((Decimal) input.Count / batchSize);
            string inputName = session.Model.InputFeatures[0].Name;
            float totalDuration = 0;
            var binding = new LearningModelBinding(session);
            for (int i = 0; i < numBatches; i++)
            {
                if (navigatingAwayFromPage)
                {
                    break;
                }

                int rangeStart = batchSize * i;
                List<VideoFrame> batch;
                // Add padding to the last batch if necessary
                if (rangeStart + batchSize > input.Count)
                {
                    int numInputsRemaining = input.Count - rangeStart;
                    int paddingAmount = batchSize - numInputsRemaining;
                    batch = input.GetRange(rangeStart, numInputsRemaining);
                    batch.AddRange(input.GetRange(0, paddingAmount));
                }
                else
                {
                    batch = input.GetRange(rangeStart, batchSize);
                }
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

        private void UpdateProgress(int attemptNumber)
        {
            EvalProgressText.Text = "Attempt " + attemptNumber.ToString() + "/" + NumEvalIterations.ToString();
            EvalProgressBar.Value = attemptNumber + 1;
        }

        private void ShowUI()
        {
            float ratio = (1 - (_avgBatchDuration / _avgNonBatchedDuration)) * 100;
            var evalResult = new EvalResult
            {
                nonBatchedAvgTime = _avgNonBatchedDuration.ToString("0.00"),
                batchedAvgTime = _avgBatchDuration.ToString("0.00"),
                timeRatio = ratio.ToString("0.0")
            };
            List<EvalResult> results = new List<EvalResult>();
            results.Insert(0, evalResult);
            LoadingContainer.Visibility = Visibility.Collapsed;
            EvalResults.Visibility = Visibility.Visible;
            StartInferenceBtn.IsEnabled = true;
            BatchSizeSlider.IsEnabled = true;
            DeviceComboBox.IsEnabled = true;
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
