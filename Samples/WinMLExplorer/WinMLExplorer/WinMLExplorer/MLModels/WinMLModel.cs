using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Windows.AI.MachineLearning.Preview;
using Windows.Graphics.Imaging;
using Windows.Media;
using Windows.Storage;
using Windows.Storage.Streams;

using WinMLExplorer.Models;
using WinMLExplorer.Utilities;

namespace WinMLExplorer.MLModels
{
    public abstract class WinMLModel
    {
        /// <summary>
        /// List of static picture files
        /// </summary>
        public IReadOnlyList<StorageFile> InputFiles { get; set; }

        /// <summary>
        /// ML model
        /// </summary>
        protected LearningModelPreview LearningModel;

        /// <summary>
        /// List of files that the model needs
        /// </summary>
        protected IReadOnlyList<StorageFile> ModelFiles;

        /// <summary>
        /// ML model file
        /// </summary>
        private StorageFile ModelFile;

        public WinMLModel()
        {
            // Initialize required model files
            Task.Run(async () => await Initialize()).Wait();
        }

        /// <summary>
        /// Name of the input (ie: circuit board) to be displayed on the main ui
        /// </summary>
        public abstract string DisplayInputName { get; }

        /// <summary>
        /// Minimum probability to be displayed on the result list
        /// </summary>
        public abstract float DisplayMinProbability { get; }

        /// <summary>
        /// Display name of the model
        /// </summary>
        public abstract string DisplayName { get; }

        /// <summary>
        /// Display result settings
        /// This is used to define the colors for each label or by probability percentage
        /// </summary>
        public abstract DisplayResultSetting [] DisplayResultSettings { get; }

        /// <summary>
        /// Model file name
        /// </summary>
        public abstract string Filename { get; }

        /// <summary>
        /// Model folder name
        /// </summary>
        public abstract string Foldername { get; }

        /// <summary>
        /// Convert a static image file to video frame
        /// </summary>
        private async Task<VideoFrame> ConvertFileToVideoFrameAsync(StorageFile file)
        {
            SoftwareBitmap softwareBitmap;
            using (IRandomAccessStream stream = await file.OpenAsync(FileAccessMode.Read))
            {
                // Create the decoder from the stream 
                BitmapDecoder decoder = await BitmapDecoder.CreateAsync(stream);

                // Get the SoftwareBitmap representation of the file 
                softwareBitmap = await decoder.GetSoftwareBitmapAsync();
                softwareBitmap = SoftwareBitmap.Convert(softwareBitmap, BitmapPixelFormat.Bgra8, BitmapAlphaMode.Ignore);
            }

            return VideoFrame.CreateWithSoftwareBitmap(softwareBitmap);
        }

        /// <summary>
        /// Evaluate static picture file
        /// </summary>
        public async Task<MLModelResult> EvaluateAsync(StorageFile inputFile)
        {
            Debug.WriteLine("Picture: " + inputFile.Name);

            using (VideoFrame inputFrame = await this.ConvertFileToVideoFrameAsync(inputFile))
            {
                return await EvaluateAsync(inputFrame);
            }
        }

        /// <summary>
        /// Evaluate video frame
        /// </summary>
        public async Task<MLModelResult> EvaluateAsync(VideoFrame inputFrame)
        {
            MLModelResult result = new MLModelResult()
            {
                CorrelationId = Guid.NewGuid().ToString()
            };

            try
            {
                // Get current datetime
                DateTime startTime = DateTime.UtcNow;

                // Evaluate
                await EvaluateAsync(result, inputFrame);

                // Set duration
                TimeSpan duration = DateTime.UtcNow - startTime;

                // Prepare result
                result.DurationInMilliSeconds = duration.TotalMilliseconds;
            }
            catch (Exception ex)
            {
                Debug.WriteLine("Exception with EvaluateAsync: " + ex);
            }

            return result;
        }

        /// <summary>
        /// Evaluate input video frame work ml model result
        /// </summary>
        protected virtual async Task EvaluateAsync(MLModelResult result, VideoFrame inputFrame)
        {
            throw new NotImplementedException();
        }

        /// <summary>
        /// Initialize the ML model
        /// </summary>
        protected virtual async Task Initialize()
        {
            string modelPath
                = Path.Combine(Windows.ApplicationModel.Package.Current.InstalledLocation.Path, "Assets", this.Foldername);

            // Get model files
            this.ModelFiles = await StorageUtility.GetFilesAsync(modelPath);
            this.ModelFile = this.ModelFiles.FirstOrDefault(file => file.Name == this.Filename);

            // Get input files
            string inputPath = Path.Combine(modelPath, "Images");
            this.InputFiles = await StorageUtility.GetFilesAsync(inputPath);

            // Initialize ML model
            await InitializeMLModel(false);
        }

        /// <summary>
        /// Initialize the ML model with isGpu defined
        /// </summary>
        private async Task InitializeMLModel(bool isGpu)
        {
            this.LearningModel = await LearningModelPreview.LoadModelFromStorageFileAsync(this.ModelFile);
            this.LearningModel.InferencingOptions.ReclaimMemoryAfterEvaluation = true;
            this.LearningModel.InferencingOptions.PreferredDeviceKind = isGpu == true ?
                LearningModelDeviceKindPreview.LearningDeviceGpu : LearningModelDeviceKindPreview.LearningDeviceCpu;
        }

        /// <summary>
        /// Set the is gpu value
        /// </summary>
        public async Task SetIsGpuValue(bool isGpu)
        {
            await InitializeMLModel(isGpu);
        }
    }
}
