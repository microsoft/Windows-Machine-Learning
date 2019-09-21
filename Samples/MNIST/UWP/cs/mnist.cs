using System;
using System.Collections.Generic;
using System.Threading.Tasks;
using Windows.Media;
using Windows.Storage;
using Windows.Storage.Streams;
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
        public enum ExtendedDeviceKind
        {
            CPU,
            GPU,
            VPU // Run MNIST on an Intel Movidius VPU, if present.
                // The VPU is not accessible via the standard LearningModelDeviceKind enumeration.
                // Instead, we use the DXCore_WinRTComponent helper.
        };

        public static ExtendedDeviceKind device_type = ExtendedDeviceKind.CPU;
        public static async Task<mnistModel> CreateFromStreamAsync(IRandomAccessStreamReference stream)
        {

            
            LearningModelDevice dev = null;
            DXCore_WinRTComponent.DXCoreHelper helper = new DXCore_WinRTComponent.DXCoreHelper();
            mnistModel learningModel = new mnistModel();
            learningModel.model = await LearningModel.LoadFromStreamAsync(stream);
            

            // Define the device based on user input
            if (mnistModel.device_type == ExtendedDeviceKind.GPU)
            {
                LearningModelDevice learningModelDevice = new Windows.AI.MachineLearning.LearningModelDevice(Windows.AI.MachineLearning.LearningModelDeviceKind.DirectX);
                learningModel.session = new LearningModelSession(learningModel.model, learningModelDevice);
            }
            else if (mnistModel.device_type == ExtendedDeviceKind.VPU)
            {
                dev = helper.GetDeviceFromVpuAdapter();
                learningModel.session = new LearningModelSession(learningModel.model, dev);
            }
            else
            {
                LearningModelDevice learningModelDevice = new Windows.AI.MachineLearning.LearningModelDevice(Windows.AI.MachineLearning.LearningModelDeviceKind.Cpu);
                learningModel.session = new LearningModelSession(learningModel.model, learningModelDevice);
            }
            
            learningModel.binding = new LearningModelBinding(learningModel.session);
            return learningModel;
        }
        public async Task<mnistOutput> EvaluateAsync(mnistInput input)
        {
            binding.Bind("Input3", input.Input3);
            var result = await session.EvaluateAsync(binding, "0");
            var output = new mnistOutput();
            output.Plus214_Output_0 = result.Outputs["Plus214_Output_0"] as TensorFloat;
            return output;
        }
    }
}
