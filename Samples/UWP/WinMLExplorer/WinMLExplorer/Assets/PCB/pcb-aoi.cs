using System;
using System.Collections.Generic;
using System.Threading.Tasks;
using Windows.Media;
using Windows.Storage;
using Windows.AI.MachineLearning.Preview;

// pcb-aoi-genimage-v2

namespace WinMLExplorer
{
    public sealed class Pcb_x002D_aoi_x002D_genimage_x002D_v2ModelInput
    {
        public VideoFrame data { get; set; }
    }

    public sealed class Pcb_x002D_aoi_x002D_genimage_x002D_v2ModelOutput
    {
        public IList<string> classLabel { get; set; }
        public IDictionary<string, IList<float>> loss { get; set; }
        public Pcb_x002D_aoi_x002D_genimage_x002D_v2ModelOutput()
        {
            this.classLabel = new List<string>();
            this.loss = new Dictionary<string, IList<float>>();
        }
    }

    public sealed class Pcb_x002D_aoi_x002D_genimage_x002D_v2Model
    {
        private LearningModelPreview learningModel;
        public static async Task<Pcb_x002D_aoi_x002D_genimage_x002D_v2Model> CreatePcb_x002D_aoi_x002D_genimage_x002D_v2Model(StorageFile file)
        {
            LearningModelPreview learningModel = await LearningModelPreview.LoadModelFromStorageFileAsync(file);
            Pcb_x002D_aoi_x002D_genimage_x002D_v2Model model = new Pcb_x002D_aoi_x002D_genimage_x002D_v2Model();
            model.learningModel = learningModel;
            return model;
        }
        public async Task<Pcb_x002D_aoi_x002D_genimage_x002D_v2ModelOutput> EvaluateAsync(Pcb_x002D_aoi_x002D_genimage_x002D_v2ModelInput input) {
            Pcb_x002D_aoi_x002D_genimage_x002D_v2ModelOutput output = new Pcb_x002D_aoi_x002D_genimage_x002D_v2ModelOutput();
            LearningModelBindingPreview binding = new LearningModelBindingPreview(learningModel);
            binding.Bind("data", input.data);
            binding.Bind("classLabel", output.classLabel);
            binding.Bind("loss", output.loss);
            LearningModelEvaluationResultPreview evalResult = await learningModel.EvaluateAsync(binding, string.Empty);
            return output;
        }
    }
}
