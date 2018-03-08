using Newtonsoft.Json;
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

using WinMLExplorer.Utilities;

namespace WinMLExplorer.Models
{
    public class ClassifierModel
    {
        public IReadOnlyList<StorageFile> InputFiles { get; set; }
        public ModelDefinition ModelDefinition { get; set; }

        private LearningModelPreview Model;
        private StorageFile ModelFile;
        private ImageVariableDescriptorPreview InputFeaturesDescriptor;
        private TensorVariableDescriptorPreview OutputFeaturesDescriptor;
        private bool? IsGpu;

        private Dictionary<string, string> Labels;
        private int SampleFilesCount = 5;

        public ClassifierModel(ModelDefinition definition)
            : base()
        {
            this.ModelDefinition = definition;

            // Initialize required model files
            Task.Run(async () => await InitializeModel()).Wait();
        }

        public async Task<ClassifierResult> EvaluateAsync(StorageFile inputFile)
        {
            using (VideoFrame inputFrame = await this.ConvertFileToVideoFrameAsync(inputFile))
            {
                return await EvaluateAsync(inputFrame);
            }
        }

        public async Task<ClassifierResult> EvaluateAsync(VideoFrame inputFrame)
        {
            ClassifierResult result = new ClassifierResult()
            {
                CorrelationId = Guid.NewGuid().ToString()
            };

            try
            {
                // Create a new output list
                //List<float> outputList = Enumerable.Range(0, this.Labels.Count).Select(index => (float)index).ToList();

                // Set duration
                TimeSpan duration = new TimeSpan();

                Pcb_x002D_aoi_x002D_genimage_x002D_v2Model m = await Pcb_x002D_aoi_x002D_genimage_x002D_v2Model.CreatePcb_x002D_aoi_x002D_genimage_x002D_v2Model(this.ModelFile);

                // Bind input and output to model
                //LearningModelBindingPreview binding = BindToModel(buffer, outputList);

                Pcb_x002D_aoi_x002D_genimage_x002D_v2ModelInput input = new Pcb_x002D_aoi_x002D_genimage_x002D_v2ModelInput() { data = inputFrame };

                DateTime startTime = DateTime.UtcNow;

                // Evaluate model
                //LearningModelEvaluationResultPreview evaluationResult = await this.Model.EvaluateAsync(binding, result.CorrelationId);

                Pcb_x002D_aoi_x002D_genimage_x002D_v2ModelOutput output = await m.EvaluateAsync(input);

                duration = DateTime.UtcNow - startTime;

                //using (SoftwareBitmap bitmapBuffer = new SoftwareBitmap(BitmapPixelFormat.Bgra8,
                //            227, 227,
                //            BitmapAlphaMode.Straight))
                //{

                //    using (VideoFrame buffer = VideoFrame.CreateWithSoftwareBitmap(bitmapBuffer))
                //    {
                //        await inputFrame.CopyToAsync(buffer);

                //        Pcb_x002D_aoi_x002D_genimage_x002D_v2Model m = await Pcb_x002D_aoi_x002D_genimage_x002D_v2Model.CreatePcb_x002D_aoi_x002D_genimage_x002D_v2Model(this.ModelFile);

                //        // Bind input and output to model
                //        //LearningModelBindingPreview binding = BindToModel(buffer, outputList);

                //        Pcb_x002D_aoi_x002D_genimage_x002D_v2ModelInput input = new Pcb_x002D_aoi_x002D_genimage_x002D_v2ModelInput() { data = buffer };

                //        DateTime startTime = DateTime.UtcNow;

                //        // Evaluate model
                //        //LearningModelEvaluationResultPreview evaluationResult = await this.Model.EvaluateAsync(binding, result.CorrelationId);

                //        Pcb_x002D_aoi_x002D_genimage_x002D_v2ModelOutput output = await m.EvaluateAsync(input);

                //        duration = DateTime.UtcNow - startTime;
                //    }
                //}

                // Prepare result
                result.DurationInMilliSeconds = duration.TotalMilliseconds;
                string classLabel = output.classLabel.FirstOrDefault();
                float probability = string.IsNullOrEmpty(classLabel) == false ? output.loss.FirstOrDefault(l => l.Key.ToLower() == classLabel.ToLower()).Value : 0f;

                result.OutputFeatures = new ClassifierFeature[]
                {
                    new ClassifierFeature()
                    {
                        Label = classLabel,
                        Probability = probability

                    }
                };

                //for (int index = 0; index < outputList.Count; index++)
                //{
                //    result.OutputFeatures[index] = new ClassifierFeature()
                //    {
                //        Label = this.Labels[index.ToString()],
                //        Probability = outputList[index]
                //    };
                //}
            }
            catch (Exception ex)
            {
                Debug.WriteLine("Exception with EvaludateAsync: " + ex);
            }

            return result;
        }

        private LearningModelBindingPreview BindToModel(object input, object output)
        {
            LearningModelBindingPreview binding = new LearningModelBindingPreview(this.Model as LearningModelPreview);
            binding.Bind(this.InputFeaturesDescriptor.Name, input);
            binding.Bind(this.OutputFeaturesDescriptor.Name, output);
            return binding;
        }

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

        private async Task InitializeModel()
        {
            string modelPath
                = Path.Combine(Windows.ApplicationModel.Package.Current.InstalledLocation.Path, "Assets", this.ModelDefinition.Foldername);

            // Get model files
            IReadOnlyList<StorageFile> modelFiles = await StorageUtility.GetFilesAsync(modelPath);
            this.ModelFile = modelFiles.FirstOrDefault(file => file.Name == this.ModelDefinition.Filename);

            // Get labels from file
            StorageFile labelsFile = modelFiles.FirstOrDefault(file => file.Name == this.ModelDefinition.LabelsFilename);
            string labelsContent = await FileIO.ReadTextAsync(labelsFile);
            this.Labels = JsonConvert.DeserializeObject<Dictionary<string, string>>(labelsContent);

            // Get input files
            string inputPath = Path.Combine(modelPath, this.ModelDefinition.InputFoldername);
            this.InputFiles = await StorageUtility.GetFilesAsync(inputPath);

            // Initialize Classifier model
            await InitializeClassifierModel(false);
        }

        private async Task InitializeClassifierModel(bool isGpu)
        {
            if (this.IsGpu.HasValue == true && this.IsGpu.Value == isGpu)
            {
                return;
            }

            // Set IsGpu value
            this.IsGpu = isGpu;

            //// Initialize model
            //this.Model = await LearningModelPreview.LoadModelFromStorageFileAsync(this.ModelFile);

            //// Default to use CPU
            //this.Model.InferencingOptions.PreferredDeviceKind =
            //    this.IsGpu == true ? LearningModelDeviceKindPreview.LearningDeviceGpu : LearningModelDeviceKindPreview.LearningDeviceCpu;

            //this.Model.InferencingOptions.ReclaimMemoryAfterEvaluation = true;

            //this.InputFeaturesDescriptor =
            //    this.Model.Description.InputFeatures.FirstOrDefault(feature => feature.ModelFeatureKind == LearningModelFeatureKindPreview.Image)
            //    as ImageVariableDescriptorPreview;

            //this.OutputFeaturesDescriptor =
            //    this.Model.Description.OutputFeatures.FirstOrDefault(feature => feature.ModelFeatureKind == LearningModelFeatureKindPreview.Tensor)
            //    as TensorVariableDescriptorPreview;
        }

        public List<string> SampleInputFilePaths
        {
            get
            {
                return this.InputFiles.Select(f => f.Path).Take(this.SampleFilesCount).ToList();
            }
        }

        public async Task SetIsGpuValue(bool isGpu)
        {
            await InitializeClassifierModel(isGpu);
        }
    }
}
