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
        public IReadOnlyList<StorageFile> InputFiles { get; set; }

        protected LearningModelPreview LearningModel;

        protected IReadOnlyList<StorageFile> ModelFiles;

        private StorageFile ModelFile;

        public WinMLModel()
        {
            // Initialize required model files
            Task.Run(async () => await Initialize()).Wait();
        }

        public abstract string DisplayInputName { get; }

        public abstract float DisplayMinProbability { get; }

        public abstract string DisplayName { get; }

        public abstract DisplayResultSetting [] DisplayResultSettings { get; }

        public abstract string Filename { get; }

        public abstract string Foldername { get; }

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

        public async Task<MLModelResult> EvaluateAsync(StorageFile inputFile)
        {
            Debug.WriteLine("Picture: " + inputFile.Name);

            using (VideoFrame inputFrame = await this.ConvertFileToVideoFrameAsync(inputFile))
            {
                return await EvaluateAsync(inputFrame);
            }
        }

        public async Task<MLModelResult> EvaluateAsync(VideoFrame inputFrame)
        {
            MLModelResult result = new MLModelResult()
            {
                CorrelationId = Guid.NewGuid().ToString()
            };

            try
            {
                // Set duration
                TimeSpan duration = new TimeSpan();

                // Get current datetime
                DateTime startTime = DateTime.UtcNow;

                // Evaluate
                await EvaluateAsync(result, inputFrame);

                duration = DateTime.UtcNow - startTime;

                // Prepare result
                result.DurationInMilliSeconds = duration.TotalMilliseconds;
            }
            catch (Exception ex)
            {
                Debug.WriteLine("Exception with EvaludateAsync: " + ex);
            }

            return result;
        }

        protected virtual async Task EvaluateAsync(MLModelResult result, VideoFrame inputFrame)
        {
            throw new NotImplementedException();
        }
        
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

        private async Task InitializeMLModel(bool isGpu)
        {
            this.LearningModel = await LearningModelPreview.LoadModelFromStorageFileAsync(this.ModelFile);
            this.LearningModel.InferencingOptions.ReclaimMemoryAfterEvaluation = true;
            this.LearningModel.InferencingOptions.PreferredDeviceKind = isGpu == true ?
                LearningModelDeviceKindPreview.LearningDeviceGpu : LearningModelDeviceKindPreview.LearningDeviceCpu;
        }

        public async Task SetIsGpuValue(bool isGpu)
        {
            await InitializeMLModel(isGpu);
        }
    }
}
