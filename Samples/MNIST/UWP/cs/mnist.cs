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
            mnistModel learningModel = new mnistModel();
            learningModel.model = await LearningModel.LoadFromStreamAsync(stream);

            // Define the device based on user input
            if (mnistModel.device_type == ExtendedDeviceKind.GPU)
            {
                LearningModelDevice gpuDevice = new Windows.AI.MachineLearning.LearningModelDevice(Windows.AI.MachineLearning.LearningModelDeviceKind.DirectX);
                learningModel.session = new LearningModelSession(learningModel.model, gpuDevice);
            }
            else if (mnistModel.device_type == ExtendedDeviceKind.VPU)
            {
                dev = DXCore_WinRTComponent.DXCoreHelper.GetDeviceFromVpuAdapter();

                // DXCoreHelper returns null if a valid device matching the requested criteria was not found.
                if (dev != null)
                {
                    learningModel.session = new LearningModelSession(learningModel.model, dev);
                }
            }

            // Either the user selected CPU, or we failed to create a LearningModelSession from a device.
            if (learningModel.session == null)
            {
                LearningModelDevice cpuDevice = new Windows.AI.MachineLearning.LearningModelDevice(Windows.AI.MachineLearning.LearningModelDeviceKind.Cpu);
                learningModel.session = new LearningModelSession(learningModel.model, cpuDevice);
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
