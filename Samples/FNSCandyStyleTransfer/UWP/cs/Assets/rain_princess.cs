using System;
using System.Collections.Generic;
using System.Threading.Tasks;
using Windows.Media;
using Windows.Storage;
using Windows.Storage.Streams;
using Windows.AI.MachineLearning;
namespace SnapCandy
{
    
    public sealed class rain_princessInput
    {
        public TensorFloat img_placeholder__0; // shape(-1,3,720,883)
    }
    
    public sealed class rain_princessOutput
    {
        public TensorFloat add_37__0; // shape(-1,3,720,884)
    }
    
    public sealed class rain_princessModel
    {
        private LearningModel model;
        private LearningModelSession session;
        private LearningModelBinding binding;
        public static async Task<rain_princessModel> CreateFromStreamAsync(IRandomAccessStreamReference stream)
        {
            rain_princessModel learningModel = new rain_princessModel();
            learningModel.model = await LearningModel.LoadFromStreamAsync(stream);
            learningModel.session = new LearningModelSession(learningModel.model);
            learningModel.binding = new LearningModelBinding(learningModel.session);
            return learningModel;
        }
        public async Task<rain_princessOutput> EvaluateAsync(rain_princessInput input)
        {
            binding.Bind("img_placeholder__0", input.img_placeholder__0);
            var result = await session.EvaluateAsync(binding, "0");
            var output = new rain_princessOutput();
            output.add_37__0 = result.Outputs["add_37__0"] as TensorFloat;
            return output;
        }
    }
}
