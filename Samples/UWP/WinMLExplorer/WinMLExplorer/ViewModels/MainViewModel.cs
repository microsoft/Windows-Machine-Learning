using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Windows.Media;
using Windows.Storage;
using Windows.UI.Xaml;
using WinMLExplorer.MLModels;
using WinMLExplorer.Models;
using WinMLExplorer.Utilities;

namespace WinMLExplorer.ViewModels
{
    public class MainViewModel : PropertyModel
    {
        /// <summary>
        /// List of supported ml models
        /// </summary>
        public List<WinMLModel> Models { get; set; }

        /// <summary>
        /// Max number of results in the result list
        /// </summary>
        private int MaxNumResults = 5;

        /// <summary>
        /// Preview video results (this is used to smooth out the UI)
        /// </summary>
        private MLModelResult PrevVideoWinMLModelResult;

        public MainViewModel()
            : base()
        {
            // Initialize models
            this.Models = new List<WinMLModel>()
            {
                new PcbModel(),
            };

            // Initialize cameras
            this.CameraNames = Task.Run(async () => await DeviceUtility.GetAvailableCameraNamesAsync()).Result.ToList();

            // Initialize results
            this.Results = new ObservableCollection<ResultViewModel>();

            // Set the first model as current model
            this.CurrentModel = this.Models.FirstOrDefault();
        }

        /// <summary>
        /// List of camera names
        /// </summary>
        public List<string> CameraNames { get; private set; }

        private WinMLModel currentModel;

        /// <summary>
        /// Current ML model
        /// </summary>
        public WinMLModel CurrentModel
        {
            get
            {
                return this.currentModel;
            }
            set
            {
                this.currentModel = value;
                this.Results.Clear();

                this.RaisePropertyChanged("DisplayFileInputName");
                this.RaisePropertyChanged("DisplayName");
                this.RaisePropertyChanged("InputFiles");
                this.RaisePropertyChanged("LandingMessage");
                this.RaisePropertyChanged("Title");
            }
        }

        /// <summary>
        /// Evaluation results to be displayed
        /// </summary>
        public ObservableCollection<ResultViewModel> Results { get; set; }

        private string duration;

        /// <summary>
        /// Evaluation duration
        /// </summary>
        public string Duration
        {
            get
            {
                return this.duration;
            }
            set
            {
                this.duration = value;
                this.RaisePropertyChanged("Duration");
            }
        }

        /// <summary>
        /// Evaluate a static picture file
        /// </summary>
        public async Task EvaluateAsync(StorageFile inputFile)
        {
            // Clear previous results
            this.Results.Clear();
            this.PrevVideoWinMLModelResult = null;

            // Evaluate input
            MLModelResult result = await this.CurrentModel.EvaluateAsync(inputFile);
            
            // Set results
            this.Duration = string.Format("{0:f1} ms", result.DurationInMilliSeconds);
            this.SetResults(result);
        }

        /// <summary>
        /// Evaluate the video frame
        /// </summary>
        public async Task EvaluateAsync(VideoFrame videoFrame)
        {
            // Evaluate input
            MLModelResult result = await this.CurrentModel.EvaluateAsync(videoFrame);

            // UI trick to smooth out transition from previous results
            if (this.PrevVideoWinMLModelResult != null
                && PrevVideoWinMLModelResult.OutputFeatures?.Count() > 0)
            {
                foreach (MLModelOutputFeature feature in result.OutputFeatures)
                {
                    MLModelOutputFeature prevFeature = this.PrevVideoWinMLModelResult.OutputFeatures.FirstOrDefault(f => f.Label == feature.Label);
                    if (prevFeature != null)
                    {
                        feature.Probability = prevFeature.Probability > 0 ? (feature.Probability + prevFeature.Probability) / 2 : feature.Probability;
                    }
                }
            }

            // Set previous result to current result
            this.PrevVideoWinMLModelResult = result;

            // Set results
            this.Duration = string.Format("{0:f1} fps", (1000 / result.DurationInMilliSeconds));
            this.SetResults(result);
        }

        /// <summary>
        /// List of input image files
        /// </summary>
        public IReadOnlyList<StorageFile> InputFiles
        {
            get
            {
                return this.CurrentModel.InputFiles;
            }
        }

        /// <summary>
        /// Display name of the input picture type
        /// </summary>
        public string DisplayFileInputName
        {
            get
            {
                return this.CurrentModel.DisplayInputName;
            }
        }

        /// <summary>
        /// Landing message to show when user opens application
        /// </summary>
        public string LandingMessage
        {
            get
            {
                return string.Format("Select {0} to start", this.DisplayFileInputName);
            }
        }

        /// <summary>
        /// Set the results to be display after evaluation
        /// </summary>
        private void SetResults(MLModelResult result)
        {
            this.SynchronizationContext.Post(_ =>
            {
                if (this.Results.Count() == 0)
                {
                    for (int index = 0; index < this.MaxNumResults; index++)
                    {
                        this.Results.Add(new ResultViewModel(this.currentModel.DisplayResultSettings));
                    }
                }

                // Sort output and filter features in descending order
                List<MLModelOutputFeature> features = result.OutputFeatures
                    .Where(f => f.Probability >= this.currentModel.DisplayMinProbability)
                    .OrderByDescending(f => f.Probability)
                    .Take(this.MaxNumResults)
                    .ToList();

                // If no result found
                if (features.Count() == 0)
                {
                    features.Add(new MLModelOutputFeature() { Label = "Nothing is found", Probability = 0 });
                }

                int featureIndex = 0;
                foreach (ResultViewModel resultViewModel in this.Results)
                {
                    if (featureIndex >= features.Count())
                    {
                        resultViewModel.IsVisible = false;
                    }
                    else
                    {
                        resultViewModel.Label = features[featureIndex].Label;
                        resultViewModel.Probability = features[featureIndex].Probability;
                        resultViewModel.IsVisible = true;
                    }

                    featureIndex++;
                }

            }, null);
        }

        /// <summary>
        /// Title of the model
        /// </summary>
        public string Title
        {
            get
            {
                return this.CurrentModel.DisplayName;
            }
        }

        /// <summary>
        /// Web camera visibility state
        /// </summary>
        public Visibility WebCameraVisibilityState => this.CameraNames?.Count() > 0 == true ? Visibility.Visible : Visibility.Collapsed;
    }
}
