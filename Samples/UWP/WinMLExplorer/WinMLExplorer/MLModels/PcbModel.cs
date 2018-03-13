using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading.Tasks;
using Windows.AI.MachineLearning.Preview;
using Windows.Foundation.Collections;
using Windows.Graphics.Imaging;
using Windows.Media;
using Windows.Storage;
using Windows.UI;

using WinMLExplorer.Models;

namespace WinMLExplorer.MLModels
{
    public sealed class PcbModelInput
    {
        public VideoFrame data { get; set; }
    }

    public sealed class PcbModelOutput
    {
        public IList<string> classLabel { get; set; }
        public IDictionary<string, float> loss { get; set; }
        public PcbModelOutput()
        {
            this.classLabel = new List<string>();
            this.loss = new Dictionary<string, float>()
            {
                { "Defective", 0f },
                { "Normal", 0f },
            };
        }
    }

    public sealed class PcbModel : WinMLModel
    {
        public override string DisplayInputName => "Circuit Board";

        public override float DisplayMinProbability => 0.1f;

        public override string DisplayName => "Circuit Boards Defects Detection";

        public override DisplayResultSetting[] DisplayResultSettings => new DisplayResultSetting[]
        {
            new DisplayResultSetting() { Name = "Normal", Color = ColorHelper.FromArgb(255, 33, 206, 114), ProbabilityRange = new Tuple<float, float>(0f, 1f) },
            new DisplayResultSetting() { Name = "Defective", Color = ColorHelper.FromArgb(255, 206, 44, 33), ProbabilityRange = new Tuple<float, float>(0f, 1f) }
        };

        public override string Filename => "pcb-aoi.onnx";

        public override string Foldername => "PCB";

        protected override async Task EvaluateAsync(MLModelResult result, VideoFrame inputFrame)
        {
            // Initialize the input
            PcbModelInput input = new PcbModelInput() { data = inputFrame };

            // Evaludate the input
            PcbModelOutput output = await EvaluateAsync(input, result.CorrelationId);
            
            // Get first label from output
            string label = output.classLabel?.FirstOrDefault();

            // Find probability for label
            if (string.IsNullOrEmpty(label) == false)
            {
                float probability = output.loss?.ContainsKey(label) == true ? output.loss[label] : 0f;

                result.OutputFeatures = new MLModelOutputFeature[]
                {
                    new MLModelOutputFeature() { Label = label, Probability = probability }
                };
            }
        }
        
        public async Task<PcbModelOutput> EvaluateAsync(PcbModelInput input, string correlationId = "") {
            PcbModelOutput output = new PcbModelOutput();

            // Bind input and output model
            LearningModelBindingPreview binding = new LearningModelBindingPreview(this.LearningModel);
            binding.Bind("data", input.data);
            binding.Bind("classLabel", output.classLabel);
            binding.Bind("loss", output.loss);

            // Evaluate the bindings
            LearningModelEvaluationResultPreview evalResult = await this.LearningModel.EvaluateAsync(binding, correlationId);
            return output;
        }
    }
}
