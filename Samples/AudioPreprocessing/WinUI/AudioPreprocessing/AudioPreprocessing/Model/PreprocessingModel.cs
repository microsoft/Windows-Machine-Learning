using Microsoft.AI.MachineLearning;
using Microsoft.AI.MachineLearning.Experimental;
using NAudio.Wave;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using Windows.Graphics.Imaging;
using Windows.Media;
using Operator = Microsoft.AI.MachineLearning.Experimental.LearningModelOperator;

namespace AudioPreprocessing.Model
{
    public class PreprocessModel
    {
        const string MicrosoftExperimentalDomain = "com.microsoft.experimental";

        public string AudioPath { get; set; }

        public string MelSpecImagePath { get; set; }

        public SoftwareBitmap GenerateMelSpectrogram(string audioPath)
        {
            var signalEnumerable = GetSignalFromFile(audioPath);
            IList<float> signal = signalEnumerable.Cast<float>().ToList();
            int batchSize = 1;
            int signalSize = signal.Count;
            int windowSize = 256;
            int dftSize = 256;
            int hopSize = 3;
            int nMelBins = 1024;
            int samplingRate = 8192;
            var rawSoftwareBitmap = GetMelspectrogramFromSignal(signal, batchSize, signalSize, windowSize, dftSize,
                hopSize, nMelBins, samplingRate);

            return rawSoftwareBitmap;
        }

        private IEnumerable<float> GetSignalFromFile(string filename, int amplitude = 5000)
        {
            using (var reader = new AudioFileReader(filename))
            {
                int rawSampleRate = reader.WaveFormat.SampleRate;
                float[] signal = new float[reader.Length];
                float[] buffer = new float[rawSampleRate];
                int read = reader.Read(buffer, 0, buffer.Length);
                int bufferCount = 0;
                while (read > 0)
                {
                    for (int i = 0; i < buffer.Length; i++)
                    {
                        signal[bufferCount * buffer.Length + i] = (buffer[i] * amplitude);
                    }
                    bufferCount += 1;
                    read = reader.Read(buffer, 0, buffer.Length);
                }

                float[] croppedSignal = new float[bufferCount * buffer.Length];
                Array.Copy(signal, croppedSignal, bufferCount * buffer.Length);
                return croppedSignal;
            }
        }

        static SoftwareBitmap GetMelspectrogramFromSignal(IEnumerable<float> rawSignal,
            int batchSize, int signalSize, int windowSize, int dftSize,
            int hopSize, int nMelBins, int samplingRate)
        {
            float[] signal = rawSignal.Cast<float>().ToArray();
            var nDfts = 1 + (signalSize - dftSize) / hopSize;
            var onesidedDftSize = (dftSize >> 1) + 1;

            long[] signalShape = { batchSize, signalSize };
            long[] melSpectrogramShape = { batchSize, 1, nDfts, nMelBins };

            var builder = LearningModelBuilder.Create(13)
                .Inputs.Add(LearningModelBuilder.CreateTensorFeatureDescriptor("Input.TimeSignal", TensorKind.Float, signalShape))
                .Outputs.Add(LearningModelBuilder.CreateTensorFeatureDescriptor("Output.MelSpectrogram", TensorKind.Float, melSpectrogramShape));
            builder
              .Operators.Add(new Operator("HannWindow", MicrosoftExperimentalDomain)
                .SetConstant("size", TensorInt64Bit.CreateFromArray(new List<long>(), new long[] { windowSize }))
                .SetOutput("output", "hann_window"));
            builder
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
                .SetConstant("shape", TensorInt64Bit.CreateFromArray(new List<long>() { 2 }, new long[] { batchSize * nDfts, onesidedDftSize }))
                .SetOutput("reshaped", "reshaped_output"))
              .Operators.Add(new Operator("MatMul")
                .SetInput("A", "reshaped_output")
                .SetInput("B", "mel_weight_matrix")
                .SetOutput("Y", "mel_spectrogram"))
              .Operators.Add(new Operator("Reshape")
                .SetInput("data", "mel_spectrogram")
                .SetConstant("shape", TensorInt64Bit.CreateFromArray(new List<long>() { 4 }, melSpectrogramShape))
                .SetOutput("reshaped", "Output.MelSpectrogram"));

            var model = builder.CreateModel();

            LearningModelSession session = new LearningModelSession(model);
            LearningModelBinding binding = new LearningModelBinding(session);

            binding.Bind("Input.TimeSignal", TensorFloat.CreateFromArray(signalShape, signal));

            // Bind output
            var outputImage = new VideoFrame(
                BitmapPixelFormat.Bgra8,
                nMelBins,
                nDfts);


            binding.Bind("Output.MelSpectrogram", outputImage);

            // Evaluate
            var sw = Stopwatch.StartNew();
            var result = session.Evaluate(binding, "");
            sw.Stop();
            Console.WriteLine("Evaluate Took: %f\n", sw.ElapsedMilliseconds);

            return outputImage.SoftwareBitmap;
        }

        static void ModelBuilding_MelWeightMatrix()
        {
            long[] outputShape = { 9, 8 };
            var builder =
                LearningModelBuilder.Create(13)
                .Outputs.Add(LearningModelBuilder.CreateTensorFeatureDescriptor("Output.MelWeightMatrix", TensorKind.Float, outputShape))
                .Operators.Add(new Operator("MelWeightMatrix", MicrosoftExperimentalDomain)
                    .SetConstant("num_mel_bins", TensorInt64Bit.CreateFromArray(new List<long>(), new long[] { 8 }))
                    .SetConstant("dft_length", TensorInt64Bit.CreateFromArray(new List<long>(), new long[] { 16 }))
                    .SetConstant("sample_rate", TensorInt64Bit.CreateFromArray(new List<long>(), new long[] { 8192 }))
                    .SetConstant("lower_edge_hertz", TensorFloat.CreateFromArray(new List<long>(), new float[] { 0 }))
                    .SetConstant("upper_edge_hertz", TensorFloat.CreateFromArray(new List<long>(), new float[] { (float)(8192 / 2.0) }))
                    .SetOutput("output", "Output.MelWeightMatrix"));

            var model = builder.CreateModel();
            LearningModelSession session = new LearningModelSession(model);
            LearningModelBinding binding = new LearningModelBinding(session);
            var result = session.Evaluate(binding, "");

            Console.WriteLine("\n");
            Console.WriteLine("Output.MelWeightMatrix\n");
        }

        static void STFT(int batchSize, int signalSize, int dftSize,
            int hopSize, int sampleRate, bool isOnesided = false)
        {
            var n_dfts = 1 + (signalSize - dftSize) / hopSize;
            var input_shape = new long[] { 1, signalSize };
            var output_shape = new long[] {
                batchSize,
                n_dfts,
                isOnesided ? (dftSize >> 1 + 1) : dftSize,
                2
            };
            var dft_length = TensorInt64Bit.CreateFromArray(new List<long>(), new long[] { dftSize });

            var builder = LearningModelBuilder.Create(13)
                .Inputs.Add(LearningModelBuilder.CreateTensorFeatureDescriptor("Input.TimeSignal", TensorKind.Float, input_shape))
                .Outputs.Add(LearningModelBuilder.CreateTensorFeatureDescriptor("Output.STFT", TensorKind.Float, output_shape))
                .Outputs.Add(LearningModelBuilder.CreateTensorFeatureDescriptor("Output.HannWindow", TensorKind.Float, new long[] { dftSize }))
                .Operators.Add(new Operator("HannWindow", MicrosoftExperimentalDomain)
                    .SetConstant("size", dft_length)
                    .SetOutput("output", "Output.HannWindow"))
                .Operators.Add(new Operator("STFT", MicrosoftExperimentalDomain)
                    .SetAttribute("onesided", TensorInt64Bit.CreateFromArray(new List<long>(), new long[] { isOnesided ? 1 : 0 }))
                    .SetInput("signal", "Input.TimeSignal")
                    .SetInput("window", "Output.HannWindow")
                    .SetConstant("frame_length", dft_length)
                    .SetConstant("frame_step", TensorInt64Bit.CreateFromArray(new List<long>(), new long[] { hopSize }))
                    .SetOutput("output", "Output.STFT"));

            var model = builder.CreateModel();

            LearningModelSession session = new LearningModelSession(model);
            LearningModelBinding binding = new LearningModelBinding(session);

            var result = session.Evaluate(binding, "");
        }

        static void WindowFunction(string windowOperatorName, TensorKind kind)
        {
            long[] scalarShape = new long[] { };
            long[] outputShape = new long[] { 32 };
            var doubleDataType = TensorInt64Bit.CreateFromArray(new List<long>() { }, new long[] { 11 });

            var windowOperator =
                new Operator(windowOperatorName, MicrosoftExperimentalDomain)
                .SetInput("size", "Input")
                .SetOutput("output", "Output");

            if (kind == TensorKind.Double)
            {
                windowOperator.SetAttribute("output_datatype", doubleDataType);
            }

            var model =
                LearningModelBuilder.Create(13)
                .Inputs.Add(LearningModelBuilder.CreateTensorFeatureDescriptor("Input", TensorKind.Int64, scalarShape))
                .Outputs.Add(LearningModelBuilder.CreateTensorFeatureDescriptor("Output", kind, outputShape))
                .Operators.Add(windowOperator)
                .CreateModel();

            LearningModelSession session = new LearningModelSession(model);
            LearningModelBinding binding = new LearningModelBinding(session);

            binding.Bind("Input", TensorInt64Bit.CreateFromArray(scalarShape, new long[] { 32 }));

            // Evaluate
            var result = session.Evaluate(binding, "");
        }

        static void ModelBuilding_HannWindow()
        {
            WindowFunction("HannWindow", TensorKind.Float);
            WindowFunction("HannWindow", TensorKind.Double);
        }
    }
}
