using Microsoft.AI.MachineLearning;
using Microsoft.AI.MachineLearning.Experimental;

namespace WinMLSamplesGallery.Samples
{
    public static class TensorizationModels {
        public enum OnnxDataType : long {
            UNDEFINED = 0,
            // Basic types.
            FLOAT = 1,
            UINT8 = 2,
            INT8 = 3,
            UINT16 = 4,
            INT16 = 5,
            INT32 = 6,
            INT64 = 7,
            STRING = 8,
            BOOL = 9,

            // IEEE754 half-precision floating-point format (16 bits wide).
            // This format has 1 sign bit, 5 exponent bits, and 10 mantissa bits.
            FLOAT16 = 10,

            DOUBLE = 11,
            UINT32 = 12,
            UINT64 = 13,
            COMPLEX64 = 14,     // complex with float32 real and imaginary components
            COMPLEX128 = 15,    // complex with float64 real and imaginary components

            // Non-IEEE floating-point format based on IEEE754 single-precision
            // floating-point number truncated to 16 bits.
            // This format has 1 sign bit, 8 exponent bits, and 7 mantissa bits.
            BFLOAT16 = 16,
        }

#pragma warning disable CA1416 // Validate platform compatibility
        public static LearningModel IdentityNCHW()
        {
            return IdentityNCHW(-1, -1, -1, -1);
        }

        public static LearningModel IdentityNCHW(long n, long c, long h, long w)
        {
            var builder = LearningModelBuilder.Create(12)
                                .Inputs.Add(LearningModelBuilder.CreateTensorFeatureDescriptor("Input", TensorKind.Float, new long[] { n, c, h, w }))
                                .Outputs.Add(LearningModelBuilder.CreateTensorFeatureDescriptor("Output", TensorKind.Float, new long[] { n, c, h, w }))
                                .Operators.Add(new LearningModelOperator("Identity")
                                                .SetInput("input", "Input")
                                                .SetOutput("output", "Output"));
            return builder.CreateModel();
        }

        public static LearningModel ShapeNCHW(long n, long c, long h, long w)
        {
            var builder = LearningModelBuilder.Create(12)
                                .Inputs.Add(LearningModelBuilder.CreateTensorFeatureDescriptor("Input", TensorKind.Float, new long[] { n, c, h, w }))
                                .Outputs.Add(LearningModelBuilder.CreateTensorFeatureDescriptor("Output", TensorKind.Int64, new long[] { -1 }))
                                .Operators.Add(new LearningModelOperator("Shape")
                                                .SetInput("data", "Input")
                                                .SetOutput("shape", "Output"));

            return builder.CreateModel();
        }

        public static LearningModel AveragePool(long kernelSize)
        {
            var kernel_shape = new long[2];
            kernel_shape[0] = kernelSize;
            kernel_shape[1] = kernelSize;
            var builder = LearningModelBuilder.Create(12)
                                .Inputs.Add(LearningModelBuilder.CreateTensorFeatureDescriptor("Input", TensorKind.Float, new long[] { -1, -1, -1, -1 }))
                                .Outputs.Add(LearningModelBuilder.CreateTensorFeatureDescriptor("Output", TensorKind.Float, new long[] { -1, -1, -1, -1 }))
                                .Operators.Add(new LearningModelOperator("AveragePool")
                                                .SetInput("X", "Input")
                                                .SetAttribute("kernel_shape", TensorInt64Bit.CreateFromArray(new long[] { kernel_shape.Length }, kernel_shape))
                                                .SetAttribute("auto_pad", TensorString.CreateFromArray(new long[] { }, new string[] { "SAME_UPPER" }))
                                                .SetOutput("Y", "Output"));
            return builder.CreateModel();
        }

        public static LearningModel Sobel()
        {
            var gx = new float[] {
                1, 0 -1, 2, 0, -2, 1, 0, -1,
                0, 0, 0, 0, 0, 0, 0, 0, 0,
                0, 0, 0, 0, 0, 0, 0, 0, 0,

                0, 0, 0, 0, 0, 0, 0, 0, 0,
                1, 0 -1, 2, 0, -2, 1, 0, -1,
                0, 0, 0, 0, 0, 0, 0, 0, 0,

                0, 0, 0, 0, 0, 0, 0, 0, 0,
                0, 0, 0, 0, 0, 0, 0, 0, 0,
                1, 0 -1, 2, 0, -2, 1, 0, -1,
            };

            var gy = new float[] {
                1, 2, 1, 0, 0, 0, -1, -2, -1,
                0, 0, 0, 0, 0, 0, 0, 0, 0,
                0, 0, 0, 0, 0, 0, 0, 0, 0,

                0, 0, 0, 0, 0, 0, 0, 0, 0,
                1, 2, 1, 0, 0, 0, -1, -2, -1,
                0, 0, 0, 0, 0, 0, 0, 0, 0,

                0, 0, 0, 0, 0, 0, 0, 0, 0,
                0, 0, 0, 0, 0, 0, 0, 0, 0,
                1, 2, 1, 0, 0, 0, -1, -2, -1,
            };

            var builder = LearningModelBuilder.Create(11)
                                .Inputs.Add(LearningModelBuilder.CreateTensorFeatureDescriptor("InputX", TensorKind.Float, new long[] { -1, 3, -1, -1 }))
                                .Outputs.Add(LearningModelBuilder.CreateTensorFeatureDescriptor("OutputX", TensorKind.Float, new long[] { -1, 3, -1, -1 }))
                                .Operators.Add(new LearningModelOperator("Conv")
                                            .SetInput("X", "InputX")
                                            .SetConstant("W", TensorFloat.CreateFromArray(new long[] { 3, 3, 3, 3 }, gx))
                                            .SetConstant("B", TensorFloat.CreateFromArray(new long[] { 1, 3, 1, 1 }, new float[] { 0, 0, 0 }))
                                            .SetAttribute("auto_pad", TensorString.CreateFromArray(new long[] { }, new string[] { "SAME_UPPER" }))
                                            .SetOutput("Y", "GradientX"))
                                .Operators.Add(new LearningModelOperator("Conv")
                                            .SetInput("X", "InputX")
                                            .SetConstant("W", TensorFloat.CreateFromArray(new long[] { 3, 3, 3, 3 }, gy))
                                            .SetConstant("B", TensorFloat.CreateFromArray(new long[] { 1, 3, 1, 1 }, new float[] { 0, 0, 0 }))
                                            .SetAttribute("auto_pad", TensorString.CreateFromArray(new long[] { }, new string[] { "SAME_UPPER" }))
                                            .SetOutput("Y", "GradientY"))
                                .Operators.Add(new LearningModelOperator("Mul")
                                                .SetInput("A", "GradientX")
                                                .SetInput("B", "GradientX")
                                                .SetOutput("C", "SquaredGradientX"))
                                .Operators.Add(new LearningModelOperator("Mul")
                                                .SetInput("A", "GradientY")
                                                .SetInput("B", "GradientY")
                                                .SetOutput("C", "SquaredGradientY"))
                                .Operators.Add(new LearningModelOperator("Add")
                                                .SetInput("A", "SquaredGradientX")
                                                .SetInput("B", "SquaredGradientY")
                                                .SetOutput("C", "GradientSquared"))
                                .Operators.Add(new LearningModelOperator("Sqrt")
                                                .SetInput("X", "GradientSquared")
                                                .SetOutput("Y", "OutputX"));
            return builder.CreateModel();
        }

        public static LearningModel AdjustBrightnessAndContrast(long n, long c, long h, long w)
        {
            var shape = new long[] { n, c, h, w };
            var builder = LearningModelBuilder.Create(11)
                                .Inputs.Add(LearningModelBuilder.CreateTensorFeatureDescriptor("Input", TensorKind.Float, shape))
                                .Inputs.Add(LearningModelBuilder.CreateTensorFeatureDescriptor("Slope", TensorKind.Float, new long[] { 1 }))
                                .Inputs.Add(LearningModelBuilder.CreateTensorFeatureDescriptor("YIntercept", TensorKind.Float, new long[] { 1 }))
                                .Outputs.Add(LearningModelBuilder.CreateTensorFeatureDescriptor("Output", TensorKind.Float, shape))
                                .Operators.Add(new LearningModelOperator("Mul")
                                                .SetInput("A", "Input")
                                                .SetInput("B", "Slope")
                                                .SetOutput("C", "MulOutput"))
                                .Operators.Add(new LearningModelOperator("Add")
                                                .SetInput("A", "MulOutput")
                                                .SetInput("B", "YIntercept")
                                                .SetOutput("C", "AddOutput"))
                                .Operators.Add(new LearningModelOperator("Clip")
                                                .SetInput("input", "AddOutput")
                                                .SetConstant("min", TensorFloat.CreateFromIterable(new long[] { 1 }, new float[] { 0 }))
                                                .SetConstant("max", TensorFloat.CreateFromIterable(new long[] { 1 }, new float[] { 255 }))
                                                .SetOutput("output", "Output"));
            return builder.CreateModel();
        }

        public static LearningModel UniformScaleAndCenterFit(long oldH, long oldW, long h, long w, string interpolationMode)
        {
            long resizedW, resizedH, top_pad, bottom_pad, left_pad, right_pad;
            CalculateCenterFitDimensions(oldH, oldW, h, w, out resizedW, out resizedH, out top_pad, out bottom_pad, out left_pad, out right_pad);
            var builder = LearningModelBuilder.Create(12)
                              .Inputs.Add(LearningModelBuilder.CreateTensorFeatureDescriptor("Input", TensorKind.Float, new long[] { -1, 3, -1, -1 }))
                              .Outputs.Add(LearningModelBuilder.CreateTensorFeatureDescriptor("Output", TensorKind.Float, new long[] { -1, 3, h, w }))
                              .Operators.Add(new LearningModelOperator("Resize")
                                              .SetInput("X", "Input")
                                              .SetConstant("roi", TensorFloat.CreateFromIterable(new long[] { 8 }, new float[] { 0, 0, 0, 0, 1, 1, 1, 1 }))
                                              .SetConstant("scales", TensorFloat.CreateFromIterable(new long[] { 4 }, new float[] { 1, 1, (float)(1+resizedH)/ (float)(oldH), (float)(1+resizedW)/ (float)oldW }))
                                              //.SetConstant("sizes", TensorInt64Bit.CreateFromIterable(new long[] { 4 }, new long[] { 1, 3, resizedH, resizedW }))
                                              // Experimental Model Building API does not support inputs of string type, so cubic interpolation cant be set...
                                              .SetAttribute("mode", TensorString.CreateFromArray(new long[] { }, new string[] { interpolationMode }))
                                              .SetOutput("Y", "ResizeOutput"))
                              .Operators.Add(new LearningModelOperator("Pad")
                                              .SetInput("data", "ResizeOutput")
                                              .SetConstant("pads", TensorInt64Bit.CreateFromIterable(new long[] { 8 }, new long[] { 0, 0, top_pad, left_pad, 0, 0, bottom_pad, right_pad }))
                                              .SetConstant("constant_value", TensorFloat.CreateFromIterable(new long[] { }, new float[] { 0.0f }))
                                              .SetOutput("output", "PadOutput"))
                                .Operators.Add(new LearningModelOperator("Slice")
                                            .SetInput("data", "PadOutput")
                                            .SetConstant("starts", TensorInt64Bit.CreateFromIterable(new long[] { 4 }, new long[] { 0, 0, 0, 0 }))
                                            .SetConstant("ends", TensorInt64Bit.CreateFromIterable(new long[] { 4 }, new long[] { long.MaxValue, long.MaxValue, h, w }))
                                            .SetOutput("output", "Output"));
            return builder.CreateModel();
        }

        public static LearningModel UniformScaleAndCenterFill(long oldH, long oldW, long h, long w, string interpolationMode)
        {
            long resizedW, resizedH, top, bottom, left, right;
            CalculateCenterFillDimensions(oldH, oldW, h, w, out resizedW, out resizedH, out top, out bottom, out left, out right);

            var builder = LearningModelBuilder.Create(12)
                                .Inputs.Add(LearningModelBuilder.CreateTensorFeatureDescriptor("Input", TensorKind.Float, new long[] { 1, 3, oldH, oldW }))
                                .Outputs.Add(LearningModelBuilder.CreateTensorFeatureDescriptor("Output", TensorKind.Float, new long[] { 1, 3, h, w }))
                                .Operators.Add(new LearningModelOperator("Resize")
                                                .SetInput("X", "Input")
                                                .SetConstant("roi", TensorFloat.CreateFromIterable(new long[] { 8 }, new float[] { 0, 0, 0, 0, 1, 1, 1, 1 }))
                                                .SetConstant("scales", TensorFloat.CreateFromIterable(new long[] { 4 }, new float[] { 1, 1, (float)(1+resizedH) / (float)oldH, (float)(1+resizedW) / (float)oldW }))
                                                //.SetConstant("sizes", TensorInt64Bit.CreateFromIterable(new long[] { 4 }, new long[] { 1, 3, resizedH, resizedW }))
                                                // Experimental Model Building API does not support inputs of string type, so cubic interpolation cant be set...
                                                .SetAttribute("mode", TensorString.CreateFromArray(new long[] { }, new string[] { interpolationMode }))
                                                .SetOutput("Y", "ResizeOutput"))
                                .Operators.Add(new LearningModelOperator("Slice")
                                            .SetInput("data", "ResizeOutput")
                                            .SetConstant("starts", TensorInt64Bit.CreateFromIterable(new long[] { 4 }, new long[] { 0, 0, top, left }))
                                            .SetConstant("ends", TensorInt64Bit.CreateFromIterable(new long[] { 4 }, new long[] { long.MaxValue, long.MaxValue, bottom, right }))
                                            .SetOutput("output", "Output"));
            return builder.CreateModel();
        }

        private static void CalculateCenterFillDimensions(long oldH, long oldW, long h, long w, out long resizedW, out long resizedH, out long top, out long bottom, out long left, out long right)
        {
            var oldHFloat = (float)oldH;
            var oldWFloat = (float)oldW;
            var hFloat = (float)h;
            var wFloat = (float)w;

            var oldAspectRatio = oldWFloat / oldHFloat;
            var newAspectRatio = wFloat / hFloat;

            var scale = (newAspectRatio < oldAspectRatio) ? (hFloat / oldHFloat) : (wFloat / oldWFloat);
            resizedW = (newAspectRatio < oldAspectRatio) ? (long)System.Math.Floor(scale * oldWFloat) : w;
            resizedH = (newAspectRatio < oldAspectRatio) ? h : (long)System.Math.Floor(scale * oldHFloat);
            long totalPad = (newAspectRatio < oldAspectRatio) ? resizedW - w: resizedH - h;
            long biggerDim = (newAspectRatio < oldAspectRatio) ? w : h;
            long first = (totalPad % 2 == 0) ? totalPad / 2 : (long)System.Math.Floor(totalPad / 2.0f);
            long second = first + biggerDim;

            if (newAspectRatio < oldAspectRatio)
            {
                top = 0;
                bottom = h;
                left = first;
                right = second;
            }
            else
            {
                top = first;
                bottom = second;
                left = 0;
                right = w;
            }
        }

        private static void CalculateCenterFitDimensions(long oldH, long oldW, long h, long w, out long resizedW, out long resizedH, out long top_pad, out long bottom_pad, out long left_pad, out long right_pad)
        {
            var oldHFloat = (float)oldH;
            var oldWFloat = (float)oldW;
            var hFloat = (float)h;
            var wFloat = (float)w;

            var oldAspectRatio = oldWFloat / oldHFloat;
            var newAspectRatio = wFloat / hFloat;

            var scale = (newAspectRatio < oldAspectRatio) ? (wFloat / oldWFloat) : (hFloat / oldHFloat);
            resizedW = (newAspectRatio < oldAspectRatio) ? w : (long)System.Math.Floor(scale * oldWFloat);
            resizedH = (newAspectRatio < oldAspectRatio) ? (long)System.Math.Floor(scale * oldHFloat) : h;
            long totalPad = (newAspectRatio < oldAspectRatio) ? h - resizedH : w - resizedW;
            long firstPad = (totalPad % 2 == 0) ? totalPad / 2 : (long)System.Math.Floor(totalPad / 2.0f);
            long secondPad = (totalPad % 2 == 0) ? firstPad : firstPad + 1;

            top_pad = 0;
            bottom_pad = 0;
            left_pad = 0;
            right_pad = 0;
            if (newAspectRatio < oldAspectRatio)
            {
                top_pad = firstPad;
                bottom_pad = secondPad;
            }
            else
            {
                left_pad = firstPad;
                right_pad = secondPad;
            }
        }

        public static LearningModel Swizzle(long first, long second, long third)
        {
            static float K(long self, long value)
            {
                return (self == value) ? 1.0f : 0.0f;
            }

            var kernel = new float[] {
                K(1, first),  K(2, first),  K(3, first),
                K(1, second), K(2, second), K(3, second),
                K(1, third),  K(2, third),  K(3, third)
            };
            var builder = LearningModelBuilder.Create(11)
                                .Inputs.Add(LearningModelBuilder.CreateTensorFeatureDescriptor("Input", TensorKind.Float, new long[] { -1, 3, -1, -1 }))
                                .Outputs.Add(LearningModelBuilder.CreateTensorFeatureDescriptor("Output", TensorKind.Float, new long[] { -1, 3, -1, -1 }))
                                 .Operators.Add(new LearningModelOperator("Conv")
                                            .SetInput("X", "Input")
                                            .SetConstant("W", TensorFloat.CreateFromArray(new long[] { 3, 3, 1, 1 }, kernel))
                                            .SetConstant("B", TensorFloat.CreateFromArray(new long[] { 1,3,1,1 }, new float[] { 0, 0, 0 }))
                                            .SetOutput("Y", "Output"));

            return builder.CreateModel();
        }

        public static LearningModel MirrorHorizontalNCHW()
        {
            var sliceStarts = new long[] { 0, 0, 0, -1 };
            var sliceEnds = new long[] { long.MaxValue, long.MaxValue, long.MaxValue, long.MinValue };
            var axes = new long[] { 0, 1, 2, 3 };
            var sliceSteps = new long[] { 1, 1, 1, -1 };
            return Slice(sliceStarts, sliceEnds, axes, sliceSteps);
        }
        public static LearningModel MirrorVerticalNCHW()
        {
            var sliceStarts = new long[] { 0, 0, -1, 0 };
            var sliceEnds = new long[] { long.MaxValue, long.MaxValue, long.MinValue, long.MaxValue };
            var axes = new long[] { 0, 1, 2, 3 };
            var sliceSteps = new long[] { 1, 1, -1, 1 };
            return Slice(sliceStarts, sliceEnds, axes, sliceSteps);
        }

        public static LearningModel RotateRight90()
        {
            return Rotate90(true /*rotateRight = false*/);
        }

        public static LearningModel RotateLeft90()
        {
            return Rotate90(false /*rotateRight = false*/);
        }
        public static LearningModel Rotate90(bool rotateRight = true)
        {
            var builder = LearningModelBuilder.Create(12)
                                .Inputs.Add(LearningModelBuilder.CreateTensorFeatureDescriptor("Input", TensorKind.Float, new long[] { -1, -1, -1, -1 }))
                                .Outputs.Add(LearningModelBuilder.CreateTensorFeatureDescriptor("Output", TensorKind.Float, new long[] { -1, -1, -1, -1 }));

            var slice = new LearningModelOperator("Slice")
                                .SetConstant("starts", TensorInt64Bit.CreateFromIterable(new long[] { 4 }, new long[] { 0, 0, 0, -1 }))
                                .SetConstant("ends", TensorInt64Bit.CreateFromIterable(new long[] { 4 }, new long[] { long.MaxValue, long.MaxValue, long.MaxValue, long.MinValue }))
                                .SetConstant("axes", TensorInt64Bit.CreateFromIterable(new long[] { 4 }, new long[] { 0, 1, 2, 3 }))
                                .SetConstant("steps", TensorInt64Bit.CreateFromIterable(new long[] { 4 }, new long[] { 1, 1, 1, -1 }));

            var transpose = new LearningModelOperator("Transpose")
                                .SetAttribute("perm", TensorInt64Bit.CreateFromArray(new long[] { 4 }, new long[] { 0, 1, 3, 2 }));

            if (rotateRight)
            {
                transpose.SetInput("data", "Input");
                transpose.SetOutput("transposed", "TransposedOutput");
                builder.Operators.Add(transpose);

                slice.SetInput("data", "TransposedOutput");
                slice.SetOutput("output", "Output");
                builder.Operators.Add(slice);
            }
            else
            {
                slice.SetInput("data", "Input");
                slice.SetOutput("output", "SliceOutput");
                builder.Operators.Add(slice);

                transpose.SetInput("data", "SliceOutput");
                transpose.SetOutput("transposed", "Output");
                builder.Operators.Add(transpose);
            }

            return builder.CreateModel();
        }

        private static LearningModel Slice(long[] sliceStarts, long[] sliceEnds, long[] axes, long[] sliceSteps)
        {
            var shape = new long[sliceStarts.Length];
            for (int i = 0; i < shape.Length; i++)
            {
                shape[i] = -1;
            }

            var builder = LearningModelBuilder.Create(12)
                                .Inputs.Add(LearningModelBuilder.CreateTensorFeatureDescriptor("Input", TensorKind.Float, shape))
                                .Outputs.Add(LearningModelBuilder.CreateTensorFeatureDescriptor("Output", TensorKind.Float, shape));

            var slice = new LearningModelOperator("Slice")
                        .SetInput("data", "Input")
                        .SetConstant("starts", TensorInt64Bit.CreateFromIterable(new long[] { sliceStarts.Length }, sliceStarts))
                        .SetConstant("ends", TensorInt64Bit.CreateFromIterable(new long[] { sliceEnds.Length }, sliceEnds))
                        .SetConstant("axes", TensorInt64Bit.CreateFromIterable(new long[] { axes.Length }, axes))
                        .SetOutput("output", "Output");

            if (sliceSteps != null)
            {
                slice.SetConstant("steps", TensorInt64Bit.CreateFromIterable(new long[] { sliceSteps.Length }, sliceSteps));
            }

            builder.Operators.Add(slice);

            return builder.CreateModel();
        }
        public static LearningModel BasicTensorization(long newH, long newW, long n, long c, long h, long w, string interpolationMode, bool castFirst = false)
        {
            long resizedW, resizedH, top, bottom, left, right;
            CalculateCenterFillDimensions(h, w, newH, newW, out resizedW, out resizedH, out top, out bottom, out left, out right);

            if (castFirst)
            {
                var builder = LearningModelBuilder.Create(11)
                                .Inputs.Add(LearningModelBuilder.CreateTensorFeatureDescriptor("Input", TensorKind.UInt8, new long[] { 1, n * c * h * w }))
                                .Outputs.Add(LearningModelBuilder.CreateTensorFeatureDescriptor("Output", TensorKind.Float, new long[] { 1, c - 1, newH, newW }))
                                    .Operators.Add(new LearningModelOperator("Cast")
                                                .SetInput("input", "Input")
                                                .SetAttribute("to", TensorInt64Bit.CreateFromIterable(new long[] { }, new long[] { (long)OnnxDataType.FLOAT }))
                                                .SetOutput("output", "CastOutput"))
                                    .Operators.Add(new LearningModelOperator("Reshape")
                                                .SetInput("data", "CastOutput")
                                                .SetConstant("shape", TensorInt64Bit.CreateFromIterable(new long[] { 4 }, new long[] { n, h, w, c }))
                                                .SetOutput("reshaped", "ReshapeOutput"))
                                    .Operators.Add(new LearningModelOperator("Resize")
                                                .SetInput("X", "ReshapeOutput")
                                                .SetConstant("roi", TensorFloat.CreateFromIterable(new long[] { 8 }, new float[] { 0, 0, 0, 0, 1, 1, 1, 1 }))
                                                .SetConstant("scales", TensorFloat.CreateFromIterable(new long[] { 4 }, new float[] { 1, (float)(1 + resizedH) / (float)h, (float)(1 + resizedH) / (float)h, 1 }))
                                                //.SetConstant("sizes", TensorInt64Bit.CreateFromIterable(new long[] { 4 }, new long[] { 1, 3, resizedH, resizedW }))
                                                // Experimental Model Building API does not support inputs of string type, so cubic interpolation cant be set...
                                                .SetAttribute("mode", TensorString.CreateFromArray(new long[] { }, new string[] { interpolationMode }))
                                                .SetOutput("Y", "ResizeOutput"))
                                    .Operators.Add(new LearningModelOperator("Slice")
                                                .SetInput("data", "ResizeOutput")
                                                .SetConstant("starts", TensorInt64Bit.CreateFromIterable(new long[] { 4 }, new long[] { 0, top, left, 0 }))
                                                .SetConstant("ends", TensorInt64Bit.CreateFromIterable(new long[] { 4 }, new long[] { long.MaxValue, bottom, right, c - 1 }))
                                                .SetOutput("output", "SliceOutput"))
                                    .Operators.Add(new LearningModelOperator("Transpose")
                                                .SetInput("data", "SliceOutput")
                                                .SetAttribute("perm", TensorInt64Bit.CreateFromArray(new long[] { 4 }, new long[] { 0, 3, 1, 2 }))
                                                .SetOutput("transposed", "Output"));

                return builder.CreateModel();
            }
            else
            {
                var builder = LearningModelBuilder.Create(11)
                                .Inputs.Add(LearningModelBuilder.CreateTensorFeatureDescriptor("Input", TensorKind.UInt8, new long[] { 1, n * c * h * w }))
                                .Outputs.Add(LearningModelBuilder.CreateTensorFeatureDescriptor("Output", TensorKind.Float, new long[] { 1, c - 1, newH, newW }))
                                    .Operators.Add(new LearningModelOperator("Reshape")
                                                .SetInput("data", "Input")
                                                .SetConstant("shape", TensorInt64Bit.CreateFromIterable(new long[] { 4 }, new long[] { n, h, w, c }))
                                                .SetOutput("reshaped", "ReshapeOutput"))
                                    .Operators.Add(new LearningModelOperator("Resize")
                                                .SetInput("X", "ReshapeOutput")
                                                .SetConstant("roi", TensorFloat.CreateFromIterable(new long[] { 8 }, new float[] { 0, 0, 0, 0, 1, 1, 1, 1 }))
                                                .SetConstant("scales", TensorFloat.CreateFromIterable(new long[] { 4 }, new float[] { 1, (float)(1 + resizedH) / (float)h, (float)(1 + resizedH) / (float)h, 1 }))
                                                //.SetConstant("sizes", TensorInt64Bit.CreateFromIterable(new long[] { 4 }, new long[] { 1, 3, resizedH, resizedW }))
                                                // Experimental Model Building API does not support inputs of string type, so cubic interpolation cant be set...
                                                .SetAttribute("mode", TensorString.CreateFromArray(new long[] { }, new string[] { interpolationMode }))
                                                .SetOutput("Y", "ResizeOutput"))
                                    .Operators.Add(new LearningModelOperator("Slice")
                                                .SetInput("data", "ResizeOutput")
                                                .SetConstant("starts", TensorInt64Bit.CreateFromIterable(new long[] { 4 }, new long[] { 0, top, left, 0 }))
                                                .SetConstant("ends", TensorInt64Bit.CreateFromIterable(new long[] { 4 }, new long[] { long.MaxValue, bottom, right, c - 1 }))
                                                .SetOutput("output", "SliceOutput"))
                                    .Operators.Add(new LearningModelOperator("Transpose")
                                                .SetInput("data", "SliceOutput")
                                                .SetAttribute("perm", TensorInt64Bit.CreateFromArray(new long[] { 4 }, new long[] { 0, 3, 1, 2 }))
                                                .SetOutput("transposed", "TransposeOutput"))
                                    .Operators.Add(new LearningModelOperator("Cast")
                                                .SetInput("input", "TransposeOutput")
                                                .SetAttribute("to", TensorInt64Bit.CreateFromIterable(new long[] { }, new long[] { (long)OnnxDataType.FLOAT }))
                                                .SetOutput("output", "Output"));

                return builder.CreateModel();
            }
        }

        public static LearningModel ReshapeFlatBufferToNCHW(long n, long c, long h, long w)
        {
            var builder = LearningModelBuilder.Create(11)
                            .Inputs.Add(LearningModelBuilder.CreateTensorFeatureDescriptor("Input", TensorKind.UInt8, new long[] { 1, n*c*h*w }))
                            .Outputs.Add(LearningModelBuilder.CreateTensorFeatureDescriptor("Output", TensorKind.Float, new long[] { n, c-1, h, w }))
                            .Operators.Add(new LearningModelOperator("Cast")
                                            .SetInput("input", "Input")
                                            .SetAttribute("to", TensorInt64Bit.CreateFromIterable(new long[] { }, new long[] { (long)OnnxDataType.FLOAT }))
                                            .SetOutput("output", "SliceOutput"))
                            .Operators.Add(new LearningModelOperator("Reshape")
                                            .SetInput("data", "SliceOutput")
                                            .SetConstant("shape", TensorInt64Bit.CreateFromIterable(new long[] { 4 }, new long[] { n, h, w, c }))
                                            .SetOutput("reshaped", "ReshapeOutput"))
                            .Operators.Add(new LearningModelOperator("Transpose")
                                            .SetInput("data", "ReshapeOutput")
                                            .SetAttribute("perm", TensorInt64Bit.CreateFromArray(new long[] { 4 }, new long[] { 0, 3, 1, 2 }))
                                            .SetOutput("transposed", "TransposeOutput"))
                            .Operators.Add(new LearningModelOperator("Slice")
                                            .SetInput("data", "TransposeOutput")
                                            .SetConstant("starts", TensorInt64Bit.CreateFromIterable(new long[] { 4 }, new long[] { 0, 0, 0, 0 }))
                                            .SetConstant("ends", TensorInt64Bit.CreateFromIterable(new long[] { 4 }, new long[] { n, c-1, h, w }))
                                            .SetOutput("output", "Output"));
            return builder.CreateModel();
        }

        public static LearningModel ReshapeFlatBufferNHWC(long n, long c, long h, long w, long newh, long neww)
        {
            long resizedW, resizedH, top, bottom, left, right;
            CalculateCenterFillDimensions(h, w, newh, neww, out resizedW, out resizedH, out top, out bottom, out left, out right);
            var builder = LearningModelBuilder.Create(11)
                            .Inputs.Add(LearningModelBuilder.CreateTensorFeatureDescriptor("Input", TensorKind.UInt8, new long[] { 1, n * c * h * w }))
                            .Outputs.Add(LearningModelBuilder.CreateTensorFeatureDescriptor("Output", TensorKind.Float, new long[] { n, newh, neww, c - 1 }))
                            .Operators.Add(new LearningModelOperator("Cast")
                                            .SetInput("input", "Input")
                                            .SetAttribute("to", TensorInt64Bit.CreateFromIterable(new long[] { }, new long[] { (long)OnnxDataType.FLOAT }))
                                            .SetOutput("output", "SliceOutput"))
                            .Operators.Add(new LearningModelOperator("Reshape")
                                            .SetInput("data", "SliceOutput")
                                            .SetConstant("shape", TensorInt64Bit.CreateFromIterable(new long[] { 4 }, new long[] { n, h, w, c }))
                                            .SetOutput("reshaped", "ReshapeOutput"))
                            .Operators.Add(new LearningModelOperator("Slice")
                                            .SetInput("data", "ReshapeOutput")
                                            .SetConstant("starts", TensorInt64Bit.CreateFromIterable(new long[] { 4 }, new long[] { 0, 0, 0, 0 }))
                                            .SetConstant("ends", TensorInt64Bit.CreateFromIterable(new long[] { 4 }, new long[] { n, h, w, c - 1 }))
                                            .SetOutput("output", "SliceOutput2"))
                            .Operators.Add(new LearningModelOperator("Resize")
                                            .SetInput("X", "SliceOutput2")
                                            .SetConstant("roi", TensorFloat.CreateFromIterable(new long[] { 8 }, new float[] { 0, 0, 0, 0, 1, 1, 1, 1 }))
                                            .SetConstant("scales", TensorFloat.CreateFromIterable(new long[] { 4 }, new float[] { 1, (float)(1 + resizedH) / (float)h, (float)(1 + w) / (float)w, 1 }))
                                            //.SetConstant("sizes", TensorInt64Bit.CreateFromIterable(new long[] { 4 }, new long[] { 1, 3, resizedH, resizedW }))
                                            // Experimental Model Building API does not support inputs of string type, so cubic interpolation cant be set...
                                            .SetAttribute("mode", TensorString.CreateFromArray(new long[] { }, new string[] { "nearest" }))
                                            .SetOutput("Y", "ResizeOutput"))
                            .Operators.Add(new LearningModelOperator("Slice")
                                        .SetInput("data", "ResizeOutput")
                                        .SetConstant("starts", TensorInt64Bit.CreateFromIterable(new long[] { 4 }, new long[] { 0, top, left, 0 }))
                                        .SetConstant("ends", TensorInt64Bit.CreateFromIterable(new long[] { 4 }, new long[] { long.MaxValue, bottom, right, long.MaxValue }))
                                        .SetOutput("output", "Output"));
            return builder.CreateModel();
        }

        //public static LearningModel NMS(long topK, float iou_threshold, float score_threshold)
        //{
        //    var builder = LearningModelBuilder.Create(11)
        //                    .Inputs.Add(LearningModelBuilder.CreateTensorFeatureDescriptor("Input", TensorKind.Float, new long[] { -1, -1, -1, 3, 85 }))
        //                    .Outputs.Add(LearningModelBuilder.CreateTensorFeatureDescriptor("Output", TensorKind.Float, new long[] { n, newh, neww, c - 1 }))
        //                    .Operators.Add(new LearningModelOperator("Cast")
        //                                    .SetInput("input", "Input")
        //                                    .SetAttribute("to", TensorInt64Bit.CreateFromIterable(new long[] { }, new long[] { (long)OnnxDataType.FLOAT }))
        //                                    .SetOutput("output", "SliceOutput"))
        //                    .Operators.Add(new LearningModelOperator("Reshape")
        //                                    .SetInput("data", "SliceOutput")
        //                                    .SetConstant("shape", TensorInt64Bit.CreateFromIterable(new long[] { 4 }, new long[] { n, h, w, c }))
        //                                    .SetOutput("reshaped", "ReshapeOutput"))
        //                    .Operators.Add(new LearningModelOperator("Slice")
        //                                    .SetInput("data", "ReshapeOutput")
        //                                    .SetConstant("starts", TensorInt64Bit.CreateFromIterable(new long[] { 4 }, new long[] { 0, 0, 0, 0 }))
        //                                    .SetConstant("ends", TensorInt64Bit.CreateFromIterable(new long[] { 4 }, new long[] { n, h, w, c - 1 }))
        //                                    .SetOutput("output", "SliceOutput2"))
        //                    .Operators.Add(new LearningModelOperator("Resize")
        //                                    .SetInput("X", "SliceOutput2")
        //                                    .SetConstant("roi", TensorFloat.CreateFromIterable(new long[] { 8 }, new float[] { 0, 0, 0, 0, 1, 1, 1, 1 }))
        //                                    .SetConstant("scales", TensorFloat.CreateFromIterable(new long[] { 4 }, new float[] { 1, (float)(1 + resizedH) / (float)h, (float)(1 + w) / (float)w, 1 }))
        //                                    //.SetConstant("sizes", TensorInt64Bit.CreateFromIterable(new long[] { 4 }, new long[] { 1, 3, resizedH, resizedW }))
        //                                    // Experimental Model Building API does not support inputs of string type, so cubic interpolation cant be set...
        //                                    .SetAttribute("mode", TensorString.CreateFromArray(new long[] { }, new string[] { "nearest" }))
        //                                    .SetOutput("Y", "ResizeOutput"))
        //                    .Operators.Add(new LearningModelOperator("Slice")
        //                                .SetInput("data", "ResizeOutput")
        //                                .SetConstant("starts", TensorInt64Bit.CreateFromIterable(new long[] { 4 }, new long[] { 0, top, left, 0 }))
        //                                .SetConstant("ends", TensorInt64Bit.CreateFromIterable(new long[] { 4 }, new long[] { long.MaxValue, bottom, right, long.MaxValue }))
        //                                .SetOutput("output", "Output"));
        //    return builder.CreateModel();
        //}

        public static LearningModel TopK(long topk)
        {
            var builder = LearningModelBuilder.Create(12)
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
            var builder = LearningModelBuilder.Create(12)
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
            var builder = LearningModelBuilder.Create(12)
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
            var builder = LearningModelBuilder.Create(12)
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
            var builder = LearningModelBuilder.Create(12)
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
                                  .SetAttribute("perm", TensorInt64Bit.CreateFromArray(new long[] { 4 }, new long[] { 0, 2, 3, 1 }))
                                  .SetOutput("transposed", "Output"));
            return builder.CreateModel();
        }
#pragma warning restore CA1416 // Validate platform compatibility
    }
}
