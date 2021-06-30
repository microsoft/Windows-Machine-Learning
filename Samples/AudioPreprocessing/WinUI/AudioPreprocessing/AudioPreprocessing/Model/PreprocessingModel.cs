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
            var rawSoftwareBitmap = GetMelspectrogramFromSignal(signal);
            return rawSoftwareBitmap;
        }

        private IEnumerable<float> GetSignalFromFile(string filename)
        {
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
            float[] signal = rawSignal.Cast<float>().ToArray();
            for (int i = 0; i < signal.Length; i++) signal[i] = signal[i] * amplitude;
            int signalSize = signal.Length;
            var nDFT = 1 + (signalSize - dftSize) / hopSize;
            var onesidedDftSize = (dftSize >> 1) + 1;

            long[] signalShape = { batchSize, signalSize };
            long[] melSpectrogramShape = { batchSize, 1, nDFT, nMelBins };

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
                .SetConstant("shape", TensorInt64Bit.CreateFromArray(new List<long>() { 2 }, new long[] { batchSize * nDFT, onesidedDftSize }))
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
                nDFT);


            binding.Bind("Output.MelSpectrogram", outputImage);

            // Evaluate
            var sw = Stopwatch.StartNew();
            var result = session.Evaluate(binding, "");
            sw.Stop();
            Console.WriteLine("Evaluate Took: %f\n", sw.ElapsedMilliseconds);

            return outputImage.SoftwareBitmap;
        }
    }
}
