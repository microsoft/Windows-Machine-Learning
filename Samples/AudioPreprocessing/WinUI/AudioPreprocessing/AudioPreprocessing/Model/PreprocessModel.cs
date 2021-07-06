using Microsoft.AI.MachineLearning;
using Microsoft.AI.MachineLearning.Experimental;
using NAudio.Wave;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Runtime.InteropServices;
using Windows.Graphics.Imaging;
using Windows.Media;
using WinRT;
using Operator = Microsoft.AI.MachineLearning.Experimental.LearningModelOperator;

namespace AudioPreprocessing.Model
{
    [ComImport]
    [Guid("5B0D3235-4DBA-4D44-865E-8F1D0E4FD04D")]
    [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    unsafe interface IMemoryBufferByteAccess
    {
        // Used to edit a SoftwareBitmap for color
        void GetBuffer(out byte* buffer, out uint capacity);
    }
    public class PreprocessModel
    {
        const string MicrosoftExperimentalDomain = "com.microsoft.experimental";

        public string AudioPath { get; set; }

        public string MelSpecImagePath { get; set; }

        public SoftwareBitmap GenerateMelSpectrogram(string audioPath, bool color = false)
        {
            var signal = GetSignalFromFile(audioPath);
            var softwareBitmap = GetMelspectrogramFromSignal(signal);
            if (color) softwareBitmap = ColorizeMelspectrogram(softwareBitmap);
            return softwareBitmap;
        }

        private IEnumerable<float> GetSignalFromFile(string filename)
        {
            if (!filename.EndsWith(".wav"))
            {
                throw new ArgumentException(String.Format("{0} is not a valid .wav file."));
            }
            using (var reader = new AudioFileReader(filename))
            {
                var nSamples = reader.Length / sizeof(float);
                var signal = Array.CreateInstance(typeof(float), nSamples);
                var read = reader.Read(signal as float[], 0, signal.Length);
                return (IEnumerable<float>)signal;
            }
        }

        static SoftwareBitmap GetMelspectrogramFromSignal(
            IEnumerable<float> rawSignal,
            int batchSize = 1,
            int windowSize = 256,
            int dftSize = 256,
            int hopSize = 3,
            int nMelBins = 1024,
            int samplingRate = 8192,
            int amplitude = 5000
            )
        {
            float[] signal = rawSignal.ToArray();

            //Scale the signal by a given amplitude 
            for (int i = 0; i < signal.Length; i++) signal[i] = signal[i] * amplitude;

            int signalSize = signal.Length;
            var nDFT = 1 + (signalSize - dftSize) / hopSize;
            var onesidedDftSize = (dftSize >> 1) + 1;

            long[] signalShape = { batchSize, signalSize };
            long[] melSpectrogramShape = { batchSize, 1, nDFT, nMelBins };

            var builder = LearningModelBuilder.Create(13)
              .Inputs.Add(LearningModelBuilder.CreateTensorFeatureDescriptor("Input.TimeSignal", TensorKind.Float, signalShape))
              .Outputs.Add(LearningModelBuilder.CreateTensorFeatureDescriptor("Output.MelSpectrogram", TensorKind.Float, melSpectrogramShape))
              .Operators.Add(new Operator("HannWindow", MicrosoftExperimentalDomain)
                .SetConstant("size", TensorInt64Bit.CreateFromArray(new List<long>(), new long[] { windowSize }))
                .SetOutput("output", "hann_window"))
              .Operators.Add(new Operator("STFT", MicrosoftExperimentalDomain)
                .SetName("STFT_NAMED_NODE")
                .SetInput("signal", "Input.TimeSignal")
                .SetInput("window", "hann_window")
                .SetConstant("frame_length", TensorInt64Bit.CreateFromArray(new List<long>(), new long[] { dftSize }))
                .SetConstant("frame_step", TensorInt64Bit.CreateFromArray(new List<long>(), new long[] { hopSize }))
                .SetOutput("output", "stft_output"))
              .Operators.Add(new Operator("ReduceSumSquare")
                .SetInput("data", "stft_output")
                .SetAttribute("axes", TensorInt64Bit.CreateFromArray(new List<long>() { 1 }, new long[] { 3 }))
                .SetAttribute("keepdims", TensorInt64Bit.CreateFromArray(new List<long>(), new long[] { 0 }))
                .SetOutput("reduced", "magnitude_squared"))
              .Operators.Add(new Operator("Div")
                .SetInput("A", "magnitude_squared")
                .SetConstant("B", TensorFloat.CreateFromArray(new List<long>(), new float[] { dftSize }))
                .SetOutput("C", "power_frames"))
              .Operators.Add(new Operator("MelWeightMatrix", MicrosoftExperimentalDomain)
                .SetConstant("num_mel_bins", TensorInt64Bit.CreateFromArray(new List<long>(), new long[] { nMelBins }))
                .SetConstant("dft_length", TensorInt64Bit.CreateFromArray(new List<long>(), new long[] { dftSize }))
                .SetConstant("sample_rate", TensorInt64Bit.CreateFromArray(new List<long>(), new long[] { samplingRate }))
                .SetConstant("lower_edge_hertz", TensorFloat.CreateFromArray(new List<long>(), new float[] { 0 }))
                .SetConstant("upper_edge_hertz", TensorFloat.CreateFromArray(new List<long>(), new float[] { (float)(samplingRate / 2.0) }))
                .SetOutput("output", "mel_weight_matrix"))
              .Operators.Add(new Operator("Reshape")
                .SetInput("data", "power_frames")
                .SetConstant("shape", TensorInt64Bit.CreateFromArray(new List<long>() { 2 }, new long[] { batchSize * nDFT, onesidedDftSize }))
                .SetOutput("reshaped", "reshaped_output"))
              .Operators.Add(new Operator("MatMul")
                .SetInput("A", "reshaped_output")
                .SetInput("B", "mel_weight_matrix")
                .SetOutput("Y", "mel_spectrogram"))
              .Operators.Add(new Operator("Reshape")
                .SetInput("data", "mel_spectrogram")
                .SetConstant("shape", TensorInt64Bit.CreateFromArray(new List<long>() { 4 }, melSpectrogramShape))
                .SetOutput("reshaped", "Output.MelSpectrogram"))
              ;

            ////colouring with LearningModelBuilder
            //builder.Operators.Add(new Operator("Slice")
            //    .SetInput("data", "bw_mel_spectrogram")
            //    .SetConstant("starts", TensorInt64Bit.CreateFromArray(new List<long>() { 3 }, new long[] { 0, 0, 0 }))
            //    .SetInput("ends", TensorInt64Bit.CreateFromArray(new List<long>() { 3 }, new long[] { 0, , 0 }))
            //    .SetConstant("shape", TensorInt64Bit.CreateFromArray(new List<long>() { 4 }, melSpectrogramShape))
            //    .SetOutput("reshaped", "hue"))
            //;

            var model = builder.CreateModel();

            LearningModelSession session = new LearningModelSession(model);
            LearningModelBinding binding = new LearningModelBinding(session);

            // Bind input
            binding.Bind("Input.TimeSignal", TensorFloat.CreateFromArray(signalShape, signal));

            // Bind output
            var outputImage = new VideoFrame(
                BitmapPixelFormat.Bgra8,
                nMelBins,
                nDFT);

            binding.Bind("Output.MelSpectrogram", outputImage);

            // Evaluate
            var sw = Stopwatch.StartNew();
            var result = session.Evaluate(binding, "");
            sw.Stop();
            Console.WriteLine("Evaluate Took: %f\n", sw.ElapsedMilliseconds);

            return outputImage.SoftwareBitmap;
        }

        public static void Colorize(Windows.Media.VideoFrame image, float saturation, float value)
        {
            long width = image.SoftwareBitmap.PixelWidth;
            long height = image.SoftwareBitmap.PixelHeight;
            long channels = image.SoftwareBitmap.BitmapPixelFormat == Windows.Graphics.Imaging.BitmapPixelFormat.Bgra8 ? 4 : 1;

            long batch_size = 1;

            var c = saturation * value;
            var m = value - saturation;


            var builder = LearningModelBuilder.Create(13)
                .Inputs.Add(LearningModelBuilder.CreateTensorFeatureDescriptor("Input", TensorKind.Float, new long[] { batch_size, channels, height, width }))
                .Outputs.Add(LearningModelBuilder.CreateTensorFeatureDescriptor("Output", TensorKind.Float, new long[] { batch_size, channels, height, width }))
                .Operators.Add(new LearningModelOperator("Slice")
                                .SetInput("data", "Input")
                                .SetConstant("starts", TensorInt64Bit.CreateFromIterable(new long[] { 4 }, new long[] { 0, 0, 0, 0 }))
                                .SetConstant("ends", TensorInt64Bit.CreateFromIterable(new long[] { 4 }, new long[] { long.MaxValue, 1, long.MaxValue, long.MaxValue }))
                                .SetConstant("axes", TensorInt64Bit.CreateFromIterable(new long[] { 4 }, new long[] { 0, 1, 2, 3 }))
                                .SetConstant("steps", TensorInt64Bit.CreateFromIterable(new long[] { 4 }, new long[] { 1, 1, 1, 1 }))
                                .SetOutput("output", "hue"))
                .Operators.Add(new LearningModelOperator("Div")
                                .SetInput("A", "hue")
                                .SetConstant("B", TensorFloat.CreateFromIterable(new long[] { 1 }, new float[] { 60 }))
                                .SetOutput("C", "div_output"))
                .Operators.Add(new LearningModelOperator("Mod")
                                .SetInput("A", "div_output")
                                .SetConstant("B", TensorFloat.CreateFromIterable(new long[] { 1 }, new float[] { 2 }))
                                .SetOutput("C", "mod_output"))
                .Operators.Add(new LearningModelOperator("Sub")
                                .SetInput("A", "mod_output")
                                .SetConstant("B", TensorFloat.CreateFromIterable(new long[] { 1 }, new float[] { 1 }))
                                .SetOutput("C", "sub1_output"))
                .Operators.Add(new LearningModelOperator("Abs")
                                .SetInput("X", "sub1_output")
                                .SetOutput("Y", "abs_output"))
                .Operators.Add(new LearningModelOperator("Sub")
                                .SetConstant("A", TensorFloat.CreateFromIterable(new long[] { 1 }, new float[] { 1 }))
                                .SetInput("B", "abs_output")
                                .SetOutput("C", "sub2_output"))
                .Operators.Add(new LearningModelOperator("Mul")
                                .SetInput("A", "sub2_output")
                                .SetConstant("B", TensorFloat.CreateFromIterable(new long[] { 1 }, new float[] { c }))
                                .SetOutput("C", "mul1_output"))
                // generate 6 masks
                .Operators.Add(new LearningModelOperator("Less")
                                .SetInput("A", "hue")
                                .SetConstant("B", TensorFloat.CreateFromIterable(new long[] { 1 }, new float[] { 60 }))
                                .SetOutput("C", "mask1"))

                .Operators.Add(new LearningModelOperator("GreaterOrEqual")
                                .SetInput("A", "hue")
                                .SetConstant("B", TensorFloat.CreateFromIterable(new long[] { 1 }, new float[] { 60 }))
                                .SetOutput("C", "greater2_output"))
                .Operators.Add(new LearningModelOperator("Less")
                                .SetInput("A", "hue")
                                .SetConstant("B", TensorFloat.CreateFromIterable(new long[] { 1 }, new float[] { 120 }))
                                .SetOutput("C", "less2_output"))
                .Operators.Add(new LearningModelOperator("Less")
                                .SetInput("A", "hue")
                                .SetConstant("B", TensorFloat.CreateFromIterable(new long[] { 1 }, new float[] { 120 }))
                                .SetOutput("C", "less2_output"))

                .Operators.Add(new LearningModelOperator("GreaterOrEqual")
                                .SetInput("A", "hue")
                                .SetConstant("B", TensorFloat.CreateFromIterable(new long[] { 1 }, new float[] { 120 }))
                                .SetOutput("C", "greater3_output"))
                .Operators.Add(new LearningModelOperator("Less")
                                .SetInput("A", "hue")
                                .SetConstant("B", TensorFloat.CreateFromIterable(new long[] { 1 }, new float[] { 180 }))
                                .SetOutput("C", "less3_output"))

                .Operators.Add(new LearningModelOperator("GreaterOrEqual")
                                .SetInput("A", "hue")
                                .SetConstant("B", TensorFloat.CreateFromIterable(new long[] { 1 }, new float[] { 180 }))
                                .SetOutput("C", "greater4_output"))
                .Operators.Add(new LearningModelOperator("Less")
                                .SetInput("A", "hue")
                                .SetConstant("B", TensorFloat.CreateFromIterable(new long[] { 1 }, new float[] { 240 }))
                                .SetOutput("C", "less4_output"))

                .Operators.Add(new LearningModelOperator("GreaterOrEqual")
                                .SetInput("A", "hue")
                                .SetConstant("B", TensorFloat.CreateFromIterable(new long[] { 1 }, new float[] { 180 }))
                                .SetOutput("C", "greater4_output"))
                .Operators.Add(new LearningModelOperator("Less")
                                .SetInput("A", "hue")
                                .SetConstant("B", TensorFloat.CreateFromIterable(new long[] { 1 }, new float[] { 240 }))
                                .SetOutput("C", "less4_output"))

                .Operators.Add(new LearningModelOperator("GreaterOrEqual")
                                .SetInput("A", "hue")
                                .SetConstant("B", TensorFloat.CreateFromIterable(new long[] { 1 }, new float[] { 300 }))
                                .SetOutput("C", "mask6"))
                ;

            builder.Save(@"C:\Users\t-janedu\source\repos\Windows-Machine-Learning\Samples\AudioPreprocessing\WinUI\AudioPreprocessing\AudioPreprocessing\tmp\colorize_me.onnx");

            var model = builder.CreateModel();
            var session = new LearningModelSession(model);
            var binding = new LearningModelBinding(session);
        }


        static unsafe SoftwareBitmap ColorizeMelspectrogram(SoftwareBitmap bwSpectrogram)
        {
            using (BitmapBuffer buffer = bwSpectrogram.LockBuffer(BitmapBufferAccessMode.Write))
            {
                using var reference = buffer.CreateReference();
                IMemoryBufferByteAccess memoryBuffer = reference.As<IMemoryBufferByteAccess>();
                memoryBuffer.GetBuffer(out byte* dataInBytes, out uint capacity);

                //HSV conversion constants
                float value = 0.5f;
                float saturation = 0.7f;
                byte c = (byte)(value * saturation * 255);
                byte m = (byte)((value - saturation) * 255);

                // Edit the BGRA Plane
                BitmapPlaneDescription bufferLayout = buffer.GetPlaneDescription(0);
                for (int i = 0; i < bufferLayout.Height; i++)
                {
                    for (int j = 0; j < bufferLayout.Width; j++)
                    {
                        int pixel = bufferLayout.StartIndex + bufferLayout.Stride * i + 4 * j;
                        int hue = dataInBytes[pixel];
                        byte x = (byte)(c * (1 - Math.Abs((hue / 60) / 2 - 1)));
                        
                        int b = pixel + 0;
                        int g = pixel + 1;
                        int r = pixel + 2;
                        int a = pixel + 3; //Alpha Layer is always at 255 for full opacity
                        switch (hue)
                        {
                            case int n when (n < 60):
                                dataInBytes[r] = c;
                                dataInBytes[g] = x;
                                dataInBytes[b] = 0;
                                break;
                            case int n when (60 <= n && n < 120):
                                dataInBytes[r] = x;
                                dataInBytes[g] = c;
                                dataInBytes[b] = 0;
                                break;                                
                            case int n when (120 <= n && n < 180):
                                dataInBytes[r] = 0;
                                dataInBytes[g] = c;
                                dataInBytes[b] = x;
                                break;
                            case int n when (180 <= n && n < 240):
                                dataInBytes[r] = 0;
                                dataInBytes[g] = x;
                                dataInBytes[b] = c;
                                break;                                
                            case int n when (240 <= n && n < 300):
                                dataInBytes[r] = x;
                                dataInBytes[g] = 0;
                                dataInBytes[b] = c;
                                break;
                            case int n when (300 <= n):
                                dataInBytes[r] = c;
                                dataInBytes[g] = 0;
                                dataInBytes[b] = x;
                                break;
                        }
                        // For the conversion, add m to all channels
                        for (int k = 0; k < 3; k++) dataInBytes[pixel + k] += m;
                        dataInBytes[a] = 255;
                    }
                }
            }
            return bwSpectrogram;
        }
    }
}
