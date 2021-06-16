using NAudio.Wave;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace AudioPreprocessing.Model
{
    public class PreprocessModel
    { 
        public PreprocessModel()
        {
            AudioPath = "../tmp/mel_spectrogram_file.jpg";
            MelSpecImagePath = "No image generated";
        }
        public string AudioPath { get; set; }
    
        public string MelSpecImagePath { get; set; }

        public string GenerateMelSpectrogram(string audioPath)
        {
            string imagePath ="";
            var signal = GetSignalFromFile(audioPath);
            imagePath = "Length of signal: " + signal.Count.ToString();

            MelSpecImagePath = imagePath;
            return MelSpecImagePath;
        }


        private List<float> GetSignalFromFile(string filename,
            int sampleRate = 8000, int amplitude = 10000)
        {
            List<float> signal = new List<float>();

            using (var reader = new AudioFileReader(filename))
            {
                float[] buffer = new float[reader.WaveFormat.SampleRate];
                int read;
                int numBuffers = 0;
                do
                {
                    read = reader.Read(buffer, 0, buffer.Length);
                    for (int n = 0; n < read; n++)
                    {
                        signal.AddRange(buffer);
                    }
                    numBuffers += 1;
                } while (read > 0);

                Console.WriteLine($"Num samples in file: {signal.Count}");
            }
            return signal;
        }

        //        static auto ReadFileWaveform(const char* filename, size_t sample_rate, size_t amplitude) {

        //    AudioFile<double> audioFile;
        //        audioFile.load(filename);
        //    size_t amp = amplitude;

        //    if (amp <= 0) {
        //        printf("Invalid amplitude for file reading. Set to default of 5000.");
        //        amp = 5000;
        //    }

        //    size_t channel = 0;
        //    size_t numSamples = audioFile.getNumSamplesPerChannel();
        //    std::vector<float> signal(numSamples);

        //    for (size_t i = 0; i<numSamples; i++)
        //    {
        //        signal[i] = audioFile.samples[channel][i] * amplitude;
        //}

        //return signal;
        //}
    }
}
