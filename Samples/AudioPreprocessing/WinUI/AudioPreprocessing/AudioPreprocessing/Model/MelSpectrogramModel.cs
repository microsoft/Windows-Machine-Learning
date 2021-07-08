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
    public class MelSpectrogramModel
    {
        const string MicrosoftExperimentalDomain = "com.microsoft.experimental";

        public string AudioPath { get; set; }

        public string MelSpecImagePath { get; set; }

        private ModelSetting melSpecSettings { get; set; } = new ModelSetting();

        private IEnumerable<float> signal { get; set; }

        public MelSpectrogramModel(ModelSetting settings)
        {
            melSpecSettings = settings;
        }

        public SoftwareBitmap GenerateMelSpectrogram(string audioPath)
        {
            GetSignalFromFile(audioPath);
            ResampleSignal();
            var softwareBitmap = GetMelspectrogramFromSignal();
            return softwareBitmap;
        }

        private void GetSignalFromFile(string filename)
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
                melSpecSettings.SourceSampleRate = reader.WaveFormat.SampleRate;
                this.signal = (IEnumerable<float>)signal;
            }
        }

        public void ResampleSignal()
        {
            //rawsamplerate
            float[] signal = this.signal.ToArray();

            int oldSampleRate = melSpecSettings.SourceSampleRate;
            int newSampleRate = melSpecSettings.SampleRate;
            int batchSize = melSpecSettings.BatchSize;
            int downsizeFactor = oldSampleRate / newSampleRate;
            int oldSignalSize = signal.Length;
            int newSignalSize = oldSignalSize / downsizeFactor;

            long[] filterSize = new long[] { 1, downsizeFactor };
            float[] filter = new float[downsizeFactor];
            for (int i = 0; i < downsizeFactor; i++) filter[i] = 1 / downsizeFactor;

            long[] inputShape = { batchSize, oldSignalSize };
            long[] outputShape = { batchSize, newSignalSize };

            var builder = LearningModelBuilder.Create(13)
              .Inputs.Add(LearningModelBuilder.CreateTensorFeatureDescriptor("Input.RawSignal", TensorKind.Float, inputShape))
              .Outputs.Add(LearningModelBuilder.CreateTensorFeatureDescriptor("Output.Resampled", TensorKind.Float, outputShape))

              .Operators.Add(new Operator("Conv")
                .SetAttribute("strides", TensorInt64Bit.CreateFromArray(new List<long>() { 2 }, new long[] { 1, downsizeFactor }))
                .SetInput("X", "Input.RawSignal")
                .SetConstant("T", TensorFloat.CreateFromArray(new List<long>() { batchSize, 1, 1, downsizeFactor }, filter))
                .SetOutput("Y", "Output.Resampled"))
              ;
            var model = builder.CreateModel();

            LearningModelSession session = new LearningModelSession(model);
            LearningModelBinding binding = new LearningModelBinding(session);

            // Bind input
            binding.Bind("Input.TimeSignal", TensorFloat.CreateFromArray(inputShape, signal));

            // Evaluate
            var sw = Stopwatch.StartNew();
            var result = session.Evaluate(binding, "");
            sw.Stop();
            Console.WriteLine("Evaluate Took: %f\n", sw.ElapsedMilliseconds);

            var obj = new Object();
            //var resampledSignal = TensorFloat.CreateFromArray(new List<long>() { batchSize, 1, 1, downsizeFactor }, new float[] { })
            result.Outputs.TryGetValue("Output.Resampled", out obj);
            var resampledSignal = TensorFloat.CreateFromArray(new List<long>() { batchSize, 1, 1, downsizeFactor }, (float[])obj);

            this.signal = resampledSignal.GetAsVectorView();
        }
   
        public SoftwareBitmap GetMelspectrogramFromSignal()
        {
            int batchSize = melSpecSettings.BatchSize;
            int windowSize = melSpecSettings.WindowSize;
            int dftSize = melSpecSettings.DFTSize;
            int hopSize = melSpecSettings.HopSize;
            int nMelBins = melSpecSettings.NMelBins;
            int samplingRate = melSpecSettings.SampleRate;
            int amplitude = melSpecSettings.Amplitude;

            float[] signal = this.signal.ToArray();

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

        public static SoftwareBitmap ColorizeWithComputationalGraph(SoftwareBitmap image, float saturation = 0.7f, float value = 0.5f)
        {
            long width = image.PixelWidth;
            long height = image.PixelHeight;
            long channels = image.BitmapPixelFormat == Windows.Graphics.Imaging.BitmapPixelFormat.Bgra8 ? 3 : 1;

            long batch_size = 1;

            var saturation_value = saturation * value;
            var saturation_value_difference = value - saturation;

            Stopwatch stopWatch = new Stopwatch();


            var builder = LearningModelBuilder.Create(13)
                .Inputs.Add(LearningModelBuilder.CreateTensorFeatureDescriptor("Input", TensorKind.Float, new long[] { batch_size, channels, height, width }))
                .Outputs.Add(LearningModelBuilder.CreateTensorFeatureDescriptor("Output", TensorKind.Float, new long[] { batch_size, channels, height, width }))
                .Operators.Add(new LearningModelOperator("Slice")
                                .SetInput("data", "Input")
                                .SetConstant("starts", TensorInt64Bit.CreateFromIterable(new long[] { 4 }, new long[] { 0, 0, 0, 0 }))
                                .SetConstant("ends", TensorInt64Bit.CreateFromIterable(new long[] { 4 }, new long[] { long.MaxValue, 1, long.MaxValue, long.MaxValue }))
                                .SetConstant("axes", TensorInt64Bit.CreateFromIterable(new long[] { 4 }, new long[] { 0, 1, 2, 3 }))
                                .SetConstant("steps", TensorInt64Bit.CreateFromIterable(new long[] { 4 }, new long[] { 1, 1, 1, 1 }))
                                .SetOutput("output", "Hue"))
                .Operators.Add(new LearningModelOperator("Div")
                                .SetInput("A", "Hue")
                                .SetConstant("B", TensorFloat.CreateFromIterable(new long[] { 1 }, new float[] { 60 }))
                                .SetOutput("C", "div_output"))
                .Operators.Add(new LearningModelOperator("Mod")
                                .SetInput("A", "div_output")
                                .SetConstant("B", TensorFloat.CreateFromIterable(new long[] { 1 }, new float[] { 2 }))
                                .SetAttribute("fmod", TensorInt64Bit.CreateFromIterable(new long[] { }, new long[] { 1 }))
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
                                .SetConstant("B", TensorFloat.CreateFromIterable(new long[] { 1 }, new float[] { saturation_value }))
                                .SetOutput("C", "X"))
                // LESS THAN MASKS
                .Operators.Add(new LearningModelOperator("Less")
                                .SetInput("A", "Hue")
                                .SetConstant("B", TensorFloat.CreateFromIterable(new long[] { 1 }, new float[] { 60 }))
                                .SetOutput("C", "lt60_mask_b"))
                .Operators.Add(new LearningModelOperator("Cast")
                                .SetInput("input", "lt60_mask_b")
                                .SetAttribute("to", TensorInt32Bit.CreateFromIterable(new long[] { }, new int[] { 1 }))
                                .SetOutput("output", "lt60_mask"))
                .Operators.Add(new LearningModelOperator("Less")
                                .SetInput("A", "Hue")
                                .SetConstant("B", TensorFloat.CreateFromIterable(new long[] { 1 }, new float[] { 120 }))
                                .SetOutput("C", "lt120_mask_b"))
                .Operators.Add(new LearningModelOperator("Cast")
                                .SetInput("input", "lt120_mask_b")
                                .SetAttribute("to", TensorInt32Bit.CreateFromIterable(new long[] { }, new int[] { 1 }))
                                .SetOutput("output", "lt120_mask"))

                .Operators.Add(new LearningModelOperator("Less")
                                .SetInput("A", "Hue")
                                .SetConstant("B", TensorFloat.CreateFromIterable(new long[] { 1 }, new float[] { 180 }))
                                .SetOutput("C", "lt180_mask_b"))
                .Operators.Add(new LearningModelOperator("Cast")
                                .SetInput("input", "lt180_mask_b")
                                .SetAttribute("to", TensorInt32Bit.CreateFromIterable(new long[] { }, new int[] { 1 }))
                                .SetOutput("output", "lt180_mask"))

                .Operators.Add(new LearningModelOperator("Less")
                                .SetInput("A", "Hue")
                                .SetConstant("B", TensorFloat.CreateFromIterable(new long[] { 1 }, new float[] { 240 }))
                                .SetOutput("C", "lt240_mask_b"))
                .Operators.Add(new LearningModelOperator("Cast")
                                .SetInput("input", "lt240_mask_b")
                                .SetAttribute("to", TensorInt32Bit.CreateFromIterable(new long[] { }, new int[] { 1 }))
                                .SetOutput("output", "lt240_mask"))

                .Operators.Add(new LearningModelOperator("Less")
                                .SetInput("A", "Hue")
                                .SetConstant("B", TensorFloat.CreateFromIterable(new long[] { 1 }, new float[] { 300 }))
                                .SetOutput("C", "lt300_mask_b"))
                .Operators.Add(new LearningModelOperator("Cast")
                                .SetInput("input", "lt300_mask_b")
                                .SetAttribute("to", TensorInt32Bit.CreateFromIterable(new long[] { }, new int[] { 1 }))
                                .SetOutput("output", "lt300_mask"))
                // GREATER THAN MASKS
                .Operators.Add(new LearningModelOperator("Greater")
                                .SetInput("A", "Hue")
                                .SetConstant("B", TensorFloat.CreateFromIterable(new long[] { 1 }, new float[] { 60 }))
                                .SetOutput("C", "gt60_mask_b"))
                .Operators.Add(new LearningModelOperator("Cast")
                                .SetInput("input", "gt60_mask_b")
                                .SetAttribute("to", TensorInt32Bit.CreateFromIterable(new long[] { }, new int[] { 1 }))
                                .SetOutput("output", "gt60_mask"))

                .Operators.Add(new LearningModelOperator("Greater")
                                .SetInput("A", "Hue")
                                .SetConstant("B", TensorFloat.CreateFromIterable(new long[] { 1 }, new float[] { 120 }))
                                .SetOutput("C", "gt120_mask_b"))
                .Operators.Add(new LearningModelOperator("Cast")
                                .SetInput("input", "gt120_mask_b")
                                .SetAttribute("to", TensorInt32Bit.CreateFromIterable(new long[] { }, new int[] { 1 }))
                                .SetOutput("output", "gt120_mask"))

                .Operators.Add(new LearningModelOperator("Greater")
                                .SetInput("A", "Hue")
                                .SetConstant("B", TensorFloat.CreateFromIterable(new long[] { 1 }, new float[] { 180 }))
                                .SetOutput("C", "gt180_mask_b"))
                .Operators.Add(new LearningModelOperator("Cast")
                                .SetInput("input", "gt180_mask_b")
                                .SetAttribute("to", TensorInt32Bit.CreateFromIterable(new long[] { }, new int[] { 1 }))
                                .SetOutput("output", "gt180_mask"))

                .Operators.Add(new LearningModelOperator("Greater")
                                .SetInput("A", "Hue")
                                .SetConstant("B", TensorFloat.CreateFromIterable(new long[] { 1 }, new float[] { 240 }))
                                .SetOutput("C", "gt240_mask_b"))
                .Operators.Add(new LearningModelOperator("Cast")
                                .SetInput("input", "gt240_mask_b")
                                .SetAttribute("to", TensorInt32Bit.CreateFromIterable(new long[] { }, new int[] { 1 }))
                                .SetOutput("output", "gt240_mask"))

                .Operators.Add(new LearningModelOperator("Greater")
                                .SetInput("A", "Hue")
                                .SetConstant("B", TensorFloat.CreateFromIterable(new long[] { 1 }, new float[] { 300 }))
                                .SetOutput("C", "gt300_mask_b"))
                .Operators.Add(new LearningModelOperator("Cast")
                                .SetInput("input", "gt300_mask_b")
                                .SetAttribute("to", TensorInt32Bit.CreateFromIterable(new long[] { }, new int[] { 1 }))
                                .SetOutput("output", "gt300_mask"))

                // CASE 0 => 60
                .Operators.Add(new LearningModelOperator("Mul")
                                .SetInput("A", "lt60_mask")
                                .SetConstant("B", TensorFloat.CreateFromIterable(new long[] { 1 }, new float[] { saturation_value }))
                                .SetOutput("C", "0_to_60_R"))
                .Operators.Add(new LearningModelOperator("Mul")
                                .SetInput("A", "lt60_mask")
                                .SetInput("B", "X")
                                .SetOutput("C", "0_to_60_G"))
                // CASE 60 => 120
                .Operators.Add(new LearningModelOperator("And")
                                .SetInput("A", "gt60_mask_b")
                                .SetInput("B", "lt120_mask_b")
                                .SetOutput("C", "60_to_120_mask_b"))
                .Operators.Add(new LearningModelOperator("Cast")
                                .SetInput("input", "60_to_120_mask_b")
                                .SetAttribute("to", TensorInt32Bit.CreateFromIterable(new long[] { }, new int[] { 1 }))
                                .SetOutput("output", "60_to_120_mask"))

                .Operators.Add(new LearningModelOperator("Mul")
                                .SetInput("A", "60_to_120_mask")
                                .SetInput("B", "X")
                                .SetOutput("C", "60_to_120_R"))
                .Operators.Add(new LearningModelOperator("Mul")
                                .SetInput("A", "60_to_120_mask")
                                .SetConstant("B", TensorFloat.CreateFromIterable(new long[] { 1 }, new float[] { saturation_value }))
                                .SetOutput("C", "60_to_120_G"))

                // CASE 120 => 180
                .Operators.Add(new LearningModelOperator("And")
                                .SetInput("A", "gt120_mask_b")
                                .SetInput("B", "lt180_mask_b")
                                .SetOutput("C", "120_to_180_mask_b"))
                .Operators.Add(new LearningModelOperator("Cast")
                                .SetInput("input", "120_to_180_mask_b")
                                .SetAttribute("to", TensorInt32Bit.CreateFromIterable(new long[] { }, new int[] { 1 }))
                                .SetOutput("output", "120_to_180_mask"))

                .Operators.Add(new LearningModelOperator("Mul")
                                .SetInput("A", "120_to_180_mask")
                                .SetConstant("B", TensorFloat.CreateFromIterable(new long[] { 1 }, new float[] { saturation_value }))
                                .SetOutput("C", "120_to_180_G"))
                .Operators.Add(new LearningModelOperator("Mul")
                                .SetInput("A", "120_to_180_mask")
                                .SetInput("B", "X")
                                .SetOutput("C", "120_to_180_B"))
                // CASE 180 => 240
                .Operators.Add(new LearningModelOperator("And")
                                .SetInput("A", "gt180_mask_b")
                                .SetInput("B", "lt240_mask_b")
                                .SetOutput("C", "180_to_240_mask_b"))
                .Operators.Add(new LearningModelOperator("Cast")
                                .SetInput("input", "180_to_240_mask_b")
                                .SetAttribute("to", TensorInt32Bit.CreateFromIterable(new long[] { }, new int[] { 1 }))
                                .SetOutput("output", "180_to_240_mask"))

                .Operators.Add(new LearningModelOperator("Mul")
                                .SetInput("A", "180_to_240_mask")
                                .SetInput("B", "X")
                                .SetOutput("C", "180_to_240_G"))
                .Operators.Add(new LearningModelOperator("Mul")
                                .SetInput("A", "180_to_240_mask")
                                .SetConstant("B", TensorFloat.CreateFromIterable(new long[] { 1 }, new float[] { saturation_value }))
                                .SetOutput("C", "180_to_240_B"))
                // CASE 240 => 300
                .Operators.Add(new LearningModelOperator("And")
                                .SetInput("A", "gt240_mask_b")
                                .SetInput("B", "lt300_mask_b")
                                .SetOutput("C", "240_to_300_mask_b"))
                .Operators.Add(new LearningModelOperator("Cast")
                                .SetInput("input", "240_to_300_mask_b")
                                .SetAttribute("to", TensorInt32Bit.CreateFromIterable(new long[] { }, new int[] { 1 }))
                                .SetOutput("output", "240_to_300_mask"))

                .Operators.Add(new LearningModelOperator("Mul")
                                .SetInput("A", "240_to_300_mask")
                                .SetInput("B", "X")
                                .SetOutput("C", "240_to_300_R"))
                .Operators.Add(new LearningModelOperator("Mul")
                                .SetInput("A", "240_to_300_mask")
                                .SetConstant("B", TensorFloat.CreateFromIterable(new long[] { 1 }, new float[] { saturation_value }))
                                .SetOutput("C", "240_to_300_B"))
                // CASE 300 => 360
                .Operators.Add(new LearningModelOperator("Mul")
                                .SetInput("A", "gt300_mask")
                                .SetConstant("B", TensorFloat.CreateFromIterable(new long[] { 1 }, new float[] { saturation_value }))
                                .SetOutput("C", "300_to_360_R"))
                .Operators.Add(new LearningModelOperator("Mul")
                                .SetInput("A", "gt300_mask")
                                .SetInput("B", "X")
                                .SetOutput("C", "300_to_360_B"))
                // SUM UP R
                .Operators.Add(new LearningModelOperator("Add")
                                .SetInput("A", "300_to_360_R")
                                .SetInput("B", "240_to_300_R")
                                .SetOutput("C", "red1"))
                .Operators.Add(new LearningModelOperator("Add")
                                .SetInput("A", "red1")
                                .SetInput("B", "60_to_120_R")
                                .SetOutput("C", "red2"))
                .Operators.Add(new LearningModelOperator("Add")
                                .SetInput("A", "red2")
                                .SetInput("B", "0_to_60_R")
                                .SetOutput("C", "red"))

                // SUM UP G
                .Operators.Add(new LearningModelOperator("Add")
                                .SetInput("A", "180_to_240_G")
                                .SetInput("B", "120_to_180_G")
                                .SetOutput("C", "green1"))
                .Operators.Add(new LearningModelOperator("Add")
                                .SetInput("A", "green1")
                                .SetInput("B", "60_to_120_G")
                                .SetOutput("C", "green2"))
                .Operators.Add(new LearningModelOperator("Add")
                                .SetInput("A", "green2")
                                .SetInput("B", "0_to_60_G")
                                .SetOutput("C", "green"))
                // SUM UP B
                .Operators.Add(new LearningModelOperator("Add")
                                .SetInput("A", "120_to_180_B")
                                .SetInput("B", "180_to_240_B")
                                .SetOutput("C", "blue1"))
                .Operators.Add(new LearningModelOperator("Add")
                                .SetInput("A", "blue1")
                                .SetInput("B", "240_to_300_B")
                                .SetOutput("C", "blue2"))
                .Operators.Add(new LearningModelOperator("Add")
                                .SetInput("A", "blue2")
                                .SetInput("B", "300_to_360_B")
                                .SetOutput("C", "blue"))

                // Create Output
                .Operators.Add(new LearningModelOperator("SequenceConstruct")
                                .SetInput("inputs", "red")
                                .SetOutput("output_sequence", "sequence_1"))
                .Operators.Add(new LearningModelOperator("SequenceInsert")
                                .SetInput("input_sequence", "sequence_1")
                                .SetInput("tensor", "green")
                                .SetOutput("output_sequence", "sequence_2"))
                .Operators.Add(new LearningModelOperator("SequenceInsert")
                                .SetInput("input_sequence", "sequence_2")
                                .SetInput("tensor", "blue")
                                .SetOutput("output_sequence", "sequence_image"))
                .Operators.Add(new LearningModelOperator("ConcatFromSequence")
                                .SetInput("input_sequence", "sequence_image")
                                .SetAttribute("axis", TensorInt64Bit.CreateFromIterable(new long[] { 1 }, new long[] { 1 }))
                                .SetOutput("concat_result", "concat_output"))

                .Operators.Add(new LearningModelOperator("Add")
                                .SetInput("A", "concat_output")
                                .SetConstant("B", TensorFloat.CreateFromIterable(new long[] { 1 }, new float[] { saturation_value_difference }))
                                .SetOutput("C", "0_1_offset"))

                .Operators.Add(new LearningModelOperator("Mul")
                                .SetInput("A", "0_1_offset")
                                .SetConstant("B", TensorFloat.CreateFromIterable(new long[] { 1 }, new float[] { 255 }))
                                .SetOutput("C", "Output"));
            ;
            builder.Save("colorize_me.onnx");

            var output_image = new VideoFrame(Windows.Graphics.Imaging.BitmapPixelFormat.Bgra8, (int)width, (int)height);

            stopWatch.Start();
            var model = builder.CreateModel();
            var device = new LearningModelDevice(LearningModelDeviceKind.Default);
            var session = new LearningModelSession(model, device);
            var binding = new LearningModelBinding(session);

            binding.Bind("Input", VideoFrame.CreateWithSoftwareBitmap(image));
            binding.Bind("Output", output_image);

            session.Evaluate(binding, "");
            stopWatch.Stop();

            TimeSpan ts = stopWatch.Elapsed;
            // Format and display the TimeSpan value.
            string elapsedTime = String.Format("{0:00}:{1:00}:{2:00}.{3:00}",
                ts.Hours, ts.Minutes, ts.Seconds,
                ts.Milliseconds / 10);
            Console.WriteLine("RunTime " + elapsedTime);
            return output_image.SoftwareBitmap;
        }

        public static unsafe void ColorizeWithBitmapEditing(SoftwareBitmap bwSpectrogram, float value = 0.5f, float saturation = 0.7f)
        {
            using (BitmapBuffer buffer = bwSpectrogram.LockBuffer(BitmapBufferAccessMode.Write))
            {
                using var reference = buffer.CreateReference();
                IMemoryBufferByteAccess memoryBuffer = reference.As<IMemoryBufferByteAccess>();
                memoryBuffer.GetBuffer(out byte* dataInBytes, out uint capacity);

                //HSV conversion constants
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
        }
    }
}
