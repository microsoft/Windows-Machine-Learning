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
                .SetOutput("reshaped", "Output.MelSpectrogram"));

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

        static unsafe SoftwareBitmap ColorizeMelspectrogram(SoftwareBitmap bwSpectrogram)
        {
            using (BitmapBuffer buffer = bwSpectrogram.LockBuffer(BitmapBufferAccessMode.Write))
            {
                using var reference = buffer.CreateReference();
                IMemoryBufferByteAccess memoryBuffer = reference.As<IMemoryBufferByteAccess>();
                memoryBuffer.GetBuffer(out byte* dataInBytes, out uint capacity);

                // Edit the BGRA Plane
                BitmapPlaneDescription bufferLayout = buffer.GetPlaneDescription(0);
                for (int i = 0; i < bufferLayout.Height; i++)
                {
                    for (int j = 0; j < bufferLayout.Width; j++)
                    {
                        int pixel = bufferLayout.StartIndex + bufferLayout.Stride * i + 4 * j;
                        //Lines below can be tweaked for different custom color filters 
                        //Blue
                        dataInBytes[pixel + 0] = (byte)((255 - dataInBytes[pixel + 0]) / 2);
                        //Green
                        dataInBytes[pixel + 1] = (byte)(dataInBytes[pixel + 1] / 2);
                        //Red
                        //dataInBytes[pixel + 2] = (byte)(dataInBytes[pixel + 2]);
                        //Alpha - must leave each pixel at max 
                        dataInBytes[bufferLayout.StartIndex + bufferLayout.Stride * i + 4 * j + 3] = (byte)255;
                    }
                }
            }
            return bwSpectrogram;
        }
    }
}
