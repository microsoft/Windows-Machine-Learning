using NAudio.Wave;
using System;
using System.Collections.Generic;
using Microsoft.AI.MachineLearning.Experimental;
using Microsoft.AI.MachineLearning;
using Windows.Media;
using Windows.Graphics.Imaging;

using Operator = Microsoft.AI.MachineLearning.Experimental.LearningModelOperator;
using System.Diagnostics;

namespace AudioPreprocessing.Model
{
    public class PreprocessModel
    {
        const string MS_EXPERIMENTAL_DOMAIN = "com.microsoft.experimental";

        public string AudioPath { get; set; }

        public string MelSpecImagePath { get; set; }

        public PreprocessModel()
        {
            AudioPath = "..\\tmp\\mel_spectrogram_file.jpg";
            MelSpecImagePath = "..\\tmp\\mel_spectrogram_file.jpg";
        }

        public SoftwareBitmap GenerateMelSpectrogram(string audioPath)
        {
            var signal = GetSignalFromFile(audioPath);
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

        private List<float> Resample(List<float> signal, int oldSampleRate, int newSampleRate)
        {
            int nStep = oldSampleRate / newSampleRate;
            var result = new List<float>();
            for (int i = 0; i < signal.Count; i += nStep)
            {
                result.Add(signal[i]);
            }
            return result;
        }


        private List<float> GetSignalFromFile(string filename, int sampleRate = 8000, int amplitude = 5000)
        {
            List<float> signal = new List<float>();
            int rawSampleRate;
            using (var reader = new AudioFileReader(filename))
            {
                rawSampleRate = reader.WaveFormat.SampleRate;
                float[] buffer = new float[rawSampleRate];
                int read = reader.Read(buffer, 0, buffer.Length);
                while (read > 0)
                {
                    foreach (float b in buffer)
                    {
                        signal.Add(b * amplitude);
                    }
                    read = reader.Read(buffer, 0, buffer.Length);
                }
            }
            Console.WriteLine($"Num samples in file: {signal.Count}");
            var newSignal = signal;
            //var newSignal = Resample(signal, rawSampleRate, sampleRate);
            Console.WriteLine($"Simple resamplein file: {signal.Count}");
            return newSignal;
        }

        static SoftwareBitmap GetMelspectrogramFromSignal(List<float> signal,
            int batchSize, int signalSize, int windowSize, int dftSize,
            int hopSize, int nMelBins, int samplingRate)
        {
            var nDfts = 1 + (signalSize - dftSize) / hopSize;
            var onesidedDftSize = (dftSize >> 1) + 1;

            long[] signalShape = { batchSize, signalSize };
            long[] melSpectrogramShape = { batchSize, 1, nDfts, nMelBins };

            var builder = LearningModelBuilder.Create(13)
                .Inputs.Add(LearningModelBuilder.CreateTensorFeatureDescriptor("Input.TimeSignal", TensorKind.Float, signalShape))
                .Outputs.Add(LearningModelBuilder.CreateTensorFeatureDescriptor("Output.MelSpectrogram", TensorKind.Float, melSpectrogramShape));
            builder
              .Operators.Add(new Operator("HannWindow", MS_EXPERIMENTAL_DOMAIN)
                .SetConstant("size", TensorInt64Bit.CreateFromArray(new List<long>(), new long[] { windowSize }))
                .SetOutput("output", "hann_window"));
            builder
              .Operators.Add(new Operator("STFT", MS_EXPERIMENTAL_DOMAIN)
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
              .Operators.Add(new Operator("MelWeightMatrix", MS_EXPERIMENTAL_DOMAIN)
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

            binding.Bind("Input.TimeSignal", TensorFloat.CreateFromArray(signalShape, signal.ToArray()));

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
            long[] output_shape = { 9, 8 };
            var builder =
                LearningModelBuilder.Create(13)
                .Outputs.Add(LearningModelBuilder.CreateTensorFeatureDescriptor("Output.MelWeightMatrix", TensorKind.Float, output_shape))
                .Operators.Add(new Operator("MelWeightMatrix", MS_EXPERIMENTAL_DOMAIN)
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

        static void STFT(int batch_size, int signal_size, int dft_size,
            int hop_size, int sample_rate, bool is_onesided = false)
        {
            var n_dfts = 1 + (signal_size - dft_size) / hop_size;
            var input_shape = new long[] { 1, signal_size };
            var output_shape = new long[] {
                batch_size,
                n_dfts,
                is_onesided ? (dft_size >> 1 + 1) : dft_size,
                2
            };
            var dft_length = TensorInt64Bit.CreateFromArray(new List<long>(), new long[] { dft_size });

            var builder = LearningModelBuilder.Create(13)
                .Inputs.Add(LearningModelBuilder.CreateTensorFeatureDescriptor("Input.TimeSignal", TensorKind.Float, input_shape))
                .Outputs.Add(LearningModelBuilder.CreateTensorFeatureDescriptor("Output.STFT", TensorKind.Float, output_shape))
                .Outputs.Add(LearningModelBuilder.CreateTensorFeatureDescriptor("Output.HannWindow", TensorKind.Float, new long[] { dft_size }))
                .Operators.Add(new Operator("HannWindow", MS_EXPERIMENTAL_DOMAIN)
                    .SetConstant("size", dft_length)
                    .SetOutput("output", "Output.HannWindow"))
                .Operators.Add(new Operator("STFT", MS_EXPERIMENTAL_DOMAIN)
                    .SetAttribute("onesided", TensorInt64Bit.CreateFromArray(new List<long>(), new long[] { is_onesided ? 1 : 0 }))
                    .SetInput("signal", "Input.TimeSignal")
                    .SetInput("window", "Output.HannWindow")
                    .SetConstant("frame_length", dft_length)
                    .SetConstant("frame_step", TensorInt64Bit.CreateFromArray(new List<long>(), new long[] { hop_size }))
                    .SetOutput("output", "Output.STFT"));

            var model = builder.CreateModel();

            LearningModelSession session = new LearningModelSession(model);
            LearningModelBinding binding = new LearningModelBinding(session);

            var result = session.Evaluate(binding, "");
        }

        static void WindowFunction(string window_operator_name, TensorKind kind)
        {
            long[] scalar_shape = new long[] { };
            long[] output_shape = new long[] { 32 };
            var double_data_type = TensorInt64Bit.CreateFromArray(new List<long>() { }, new long[] { 11 });

            var window_operator =
                new Operator(window_operator_name, MS_EXPERIMENTAL_DOMAIN)
                .SetInput("size", "Input")
                .SetOutput("output", "Output");

            if (kind == TensorKind.Double)
            {
                window_operator.SetAttribute("output_datatype", double_data_type);
            }

            var model =
                LearningModelBuilder.Create(13)
                .Inputs.Add(LearningModelBuilder.CreateTensorFeatureDescriptor("Input", TensorKind.Int64, scalar_shape))
                .Outputs.Add(LearningModelBuilder.CreateTensorFeatureDescriptor("Output", kind, output_shape))
                .Operators.Add(window_operator)
                .CreateModel();

            LearningModelSession session = new LearningModelSession(model);
            LearningModelBinding binding = new LearningModelBinding(session);

            binding.Bind("Input", TensorInt64Bit.CreateFromArray(scalar_shape, new long[] { 32 }));

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
