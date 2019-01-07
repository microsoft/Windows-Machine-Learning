using System;
using System.Collections.Generic;
using System.Threading.Tasks;
using Windows.Media;
using Windows.Storage;
using Windows.Storage.Streams;
using Windows.AI.MachineLearning;
namespace SqueezeNetObjectDetection
{
    
    public sealed class modelInput
    {
        public TensorFloat data_0; // shape(1,3,224,224)
    }
    
    public sealed class modelOutput
    {
        public TensorFloat softmaxout_1; // shape(1,1000,1,1)
    }
    
    public sealed class modelModel
    {
        private LearningModel model;
        private LearningModelSession session;
        private LearningModelBinding binding;
        public static async Task<modelModel> CreateFromStreamAsync(IRandomAccessStreamReference stream)
        {
            modelModel learningModel = new modelModel();
            learningModel.model = await LearningModel.LoadFromStreamAsync(stream);
            learningModel.session = new LearningModelSession(learningModel.model);
            learningModel.binding = new LearningModelBinding(learningModel.session);
            return learningModel;
        }
        public async Task<modelOutput> EvaluateAsync(modelInput input)
        {
            binding.Bind("data_0", input.data_0);
            var result = await session.EvaluateAsync(binding, "0");
            var output = new modelOutput();
            output.softmaxout_1 = result.Outputs["softmaxout_1"] as TensorFloat;
            return output;
        }
    }
}
