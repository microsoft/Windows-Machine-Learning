using System;
using System.Collections.Generic;
using System.Threading.Tasks;
using Windows.Media;
using Windows.Storage;
using Windows.AI.MachineLearning;
namespace MNIST_Demo
{
    
    public sealed class mnistInput
    {
        public ImageFeatureValue Input3; // BitmapPixelFormat: Gray8, BitmapAlphaMode: Premultiplied, width: 28, height: 28
    }
    
    public sealed class mnistOutput
    {
        public TensorFloat Plus214_Output_0; // shape(1,10)
    }
    
    public sealed class mnistModel
    {
        private LearningModel model;
        private LearningModelSession session;
        private LearningModelBinding binding;
        public static mnistModel CreateFromFilePath(string path)
        {
            mnistModel learningModel = new mnistModel();
            learningModel.model = LearningModel.LoadFromFilePath(path);
            learningModel.session = new LearningModelSession(learningModel.model);
            learningModel.binding = new LearningModelBinding(learningModel.session);
            return learningModel;
        }
        public async Task<mnistOutput> Evaluate(mnistInput input)
        {
            binding.Bind("Input3", input.Input3);
            var result = await session.EvaluateAsync(binding, "0");
            var output = new mnistOutput();
            output.Plus214_Output_0 = result.Outputs["Plus214_Output_0"] as TensorFloat;
            return output;
        }
    }
}
