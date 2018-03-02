using System;
using System.Collections.Generic;
using System.Threading.Tasks;
using Windows.Media;
using Windows.Storage;
using Windows.AI.MachineLearning.Preview;

// MNIST

namespace MNIST_Demo
{
    public sealed class MNISTModelInput
    {
        public VideoFrame Input3 { get; set; }
    }

    public sealed class MNISTModelOutput
    {
        public IList<float> Plus214_Output_0 { get; set; }
        public MNISTModelOutput()
        {
            this.Plus214_Output_0 = new List<float>();
        }
    }

    public sealed class MNISTModel
    {
        private LearningModelPreview learningModel;
        public static async Task<MNISTModel> CreateMNISTModel(StorageFile file)
        {
            LearningModelPreview learningModel = await LearningModelPreview.LoadModelFromStorageFileAsync(file);
            MNISTModel model = new MNISTModel();
            model.learningModel = learningModel;
            return model;
        }
        public async Task<MNISTModelOutput> EvaluateAsync(MNISTModelInput input) {
            MNISTModelOutput output = new MNISTModelOutput();
            LearningModelBindingPreview binding = new LearningModelBindingPreview(learningModel);
            binding.Bind("Input3", input.Input3);
            binding.Bind("Plus214_Output_0", output.Plus214_Output_0);
            LearningModelEvaluationResultPreview evalResult = await learningModel.EvaluateAsync(binding, string.Empty);
            return output;
        }
    }
}
