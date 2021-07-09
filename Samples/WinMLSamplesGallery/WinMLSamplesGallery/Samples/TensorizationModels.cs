using Microsoft.AI.MachineLearning;
using Microsoft.AI.MachineLearning.Experimental;

namespace WinMLSamplesGallery.Samples
{
    public static class TensorizationModels {
#pragma warning disable CA1416 // Validate platform compatibility
        public static LearningModel TopK(long topk)
        {
            var builder = LearningModelBuilder.Create(13)
                            .Inputs.Add(LearningModelBuilder.CreateTensorFeatureDescriptor("ClassifierOutput", TensorKind.Float, new long[] { -1, -1 }))
                            .Outputs.Add(LearningModelBuilder.CreateTensorFeatureDescriptor("TopKValues", TensorKind.Float, new long[] { -1, topk }))
                            .Outputs.Add(LearningModelBuilder.CreateTensorFeatureDescriptor("TopKIndices", TensorKind.Int64, new long[] { -1, topk }))
                            .Operators.Add(new LearningModelOperator("TopK")
                                            .SetInput("X", "ClassifierOutput")
                                            .SetConstant("K", TensorInt64Bit.CreateFromIterable(new long[] { 1 }, new long[] { topk }))
                                            .SetAttribute("largest", TensorInt64Bit.CreateFromIterable(new long[] { }, new long[] { 1 }))
                                            .SetAttribute("axis", TensorInt64Bit.CreateFromIterable(new long[] { }, new long[] { -1 }))
                                            .SetOutput("Values", "TopKValues")
                                            .SetOutput("Indices", "TopKIndices"));

            return builder.CreateModel();
        }

        public static LearningModel ReshapeThenSoftmaxThenTopK(long[] inputShape, long topk, long batchSize, long nClasses)
        {
            var builder = LearningModelBuilder.Create(13)
                            .Inputs.Add(LearningModelBuilder.CreateTensorFeatureDescriptor("ClassifierOutput", TensorKind.Float, inputShape))
                            .Outputs.Add(LearningModelBuilder.CreateTensorFeatureDescriptor("TopKValues", TensorKind.Float, new long[] { batchSize, topk }))
                            .Outputs.Add(LearningModelBuilder.CreateTensorFeatureDescriptor("TopKIndices", TensorKind.Int64, new long[] { batchSize, topk }))
                            .Operators.Add(new LearningModelOperator("Reshape")
                                            .SetInput("data", "ClassifierOutput")
                                            .SetConstant("shape", TensorInt64Bit.CreateFromIterable(new long[] { 2 }, new long[] { batchSize, nClasses }))
                                            .SetOutput("reshaped", "reshaped_output"))
                            .Operators.Add(new LearningModelOperator("Softmax")
                                            .SetInput("input", "reshaped_output")
                                            .SetAttribute("axis", TensorInt64Bit.CreateFromIterable(new long[] { }, new long[] { -1 }))
                                            .SetOutput("output", "softmax_output"))
                            .Operators.Add(new LearningModelOperator("TopK")
                                            .SetInput("X", "softmax_output")
                                            .SetConstant("K", TensorInt64Bit.CreateFromIterable(new long[] { 1 }, new long[] { topk }))
                                            .SetAttribute("largest", TensorInt64Bit.CreateFromIterable(new long[] { }, new long[] { 1 }))
                                            .SetAttribute("axis", TensorInt64Bit.CreateFromIterable(new long[] { }, new long[] { -1 }))
                                            .SetOutput("Values", "TopKValues")
                                            .SetOutput("Indices", "TopKIndices"));

            return builder.CreateModel();
        }

        public static LearningModel SoftMaxThenTopK(long topk)
        {
            var builder = LearningModelBuilder.Create(13)
                            .Inputs.Add(LearningModelBuilder.CreateTensorFeatureDescriptor("ClassifierOutput", TensorKind.Float, new long[] { -1, -1 }))
                            .Outputs.Add(LearningModelBuilder.CreateTensorFeatureDescriptor("TopKValues", TensorKind.Float, new long[] { -1, topk }))
                            .Outputs.Add(LearningModelBuilder.CreateTensorFeatureDescriptor("TopKIndices", TensorKind.Int64, new long[] { -1, topk }))
                            .Operators.Add(new LearningModelOperator("Softmax")
                                            .SetInput("input", "ClassifierOutput")
                                            .SetAttribute("axis", TensorInt64Bit.CreateFromIterable(new long[] { }, new long[] { -1 }))
                                            .SetOutput("output", "softmax_output"))
                            .Operators.Add(new LearningModelOperator("TopK")
                                            .SetInput("X", "softmax_output")
                                            .SetConstant("K", TensorInt64Bit.CreateFromIterable(new long[] { 1 }, new long[] { topk }))
                                            .SetAttribute("largest", TensorInt64Bit.CreateFromIterable(new long[] { }, new long[] { 1 }))
                                            .SetAttribute("axis", TensorInt64Bit.CreateFromIterable(new long[] { }, new long[] { -1 }))
                                            .SetOutput("Values", "TopKValues")
                                            .SetOutput("Indices", "TopKIndices"));

            return builder.CreateModel();
        }

        public static LearningModel Normalize0_1ThenZScore(long height, long width, long channels, float[] means, float[] stddev) {
            System.Diagnostics.Debug.Assert(means.Length == channels - 1);
            System.Diagnostics.Debug.Assert(stddev.Length == channels - 1);

            // Since this model is expected to be used with VideoFrame input,
            // There is no need to manually transform NHWC to NCHW or Slice off the alpha dimension.
            var inputShape = new long[]{ 1, channels - 1, height, width };
            var outputShape = new long[] { 1, channels - 1, height, width };
            var builder = LearningModelBuilder.Create(13)
              .Inputs.Add(LearningModelBuilder.CreateTensorFeatureDescriptor("Input", "The NCHW image", TensorKind.Float, inputShape))
              .Outputs.Add(LearningModelBuilder.CreateTensorFeatureDescriptor("Output", "The NCHW image normalized with mean and stddev.", TensorKind.Float, outputShape))
              .Operators.Add(new LearningModelOperator("Div") // Normalize from 0-255 to 0-1 by dividing by 255
                                  .SetInput("A", "Input")
                                  .SetConstant("B", TensorFloat.CreateFromArray(new long[] { }, new float[] { 255f }))
                                  .SetOutput("C", "DivOutput"))
              .Operators.Add(new LearningModelOperator("Reshape")
                                  .SetConstant("data", TensorFloat.CreateFromArray(new long[] { means.Length }, means))
                                  .SetConstant("shape", TensorInt64Bit.CreateFromIterable(new long[] { 4 }, new long[] { 1, channels - 1, 1, 1 }))
                                  .SetOutput("reshaped", "MeansReshaped"))
              .Operators.Add(new LearningModelOperator("Reshape")
                                  .SetConstant("data", TensorFloat.CreateFromArray(new long[] { stddev.Length }, stddev))
                                  .SetConstant("shape", TensorInt64Bit.CreateFromIterable(new long[] { 4 }, new long[] { 1, channels - 1, 1, 1 }))
                                  .SetOutput("reshaped", "StdDevReshaped"))
              .Operators.Add(new LearningModelOperator("Sub") // Shift by the means
                                  .SetInput("A", "DivOutput")
                                  .SetInput("B", "MeansReshaped")
                                  .SetOutput("C", "SubOutput"))
              .Operators.Add(new LearningModelOperator("Div")  // Divide by stddev
                                  .SetInput("A", "SubOutput")
                                  .SetInput("B", "StdDevReshaped")
                                  .SetOutput("C", "Output"));

            return builder.CreateModel();
        }
        public static LearningModel NormalizeMinusOneToOneThenTransposeNHWC()
        {
            var inputShape = new long[] { 1, 3, 224, 224 };
            var outputShape = new long[] { 1, -1, -1, 3 };
            var builder = LearningModelBuilder.Create(13)
              .Inputs.Add(LearningModelBuilder.CreateTensorFeatureDescriptor("Input", "The image", TensorKind.Float, inputShape))
              .Outputs.Add(LearningModelBuilder.CreateTensorFeatureDescriptor("Output", "The -1 to 1 normalized image.", TensorKind.Float, outputShape))
              .Operators.Add(new LearningModelOperator("Div") // Normalize from 0-255 to 0-2 by dividing by 255/2
                                  .SetInput("A", "Input")
                                  .SetConstant("B", TensorFloat.CreateFromArray(new long[] { }, new float[] { 255f/2f }))
                                  .SetOutput("C", "DivOutput"))
              .Operators.Add(new LearningModelOperator("Sub") // Normalize from 0-2 to -1-1 by shifting down by 1
                                  .SetInput("A", "DivOutput")
                                  .SetConstant("B", TensorFloat.CreateFromArray(new long[] { }, new float[] { 1f }))
                                  .SetOutput("C", "SubOutput"))
              .Operators.Add(new LearningModelOperator("Transpose") // Move the output back to NHWC (this is what efficientnet requires)
                                  .SetInput("data", "SubOutput")
                                  .SetAttribute("perm", TensorInt64Bit.CreateFromArray(new long[] { 4 }, new long[] { 0, 3, 2, 1 }))
                                  .SetOutput("transposed", "Output"));
            return builder.CreateModel();
        }
#pragma warning restore CA1416 // Validate platform compatibility
    }
}
