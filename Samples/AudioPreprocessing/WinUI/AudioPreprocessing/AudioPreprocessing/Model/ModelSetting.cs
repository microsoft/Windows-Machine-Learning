using System;
using System.Collections.Generic;
using System.Text;

namespace AudioPreprocessing.Model
{
    public class ModelSetting
    {
        public bool Colored { get; set; }
        public int BatchSize { get; set; }
        public int WindowSize { get; set; }
        public int DFTSize { get; set; }
        public int HopSize { get; set; }
        public int NMelBins { get; set; }
        public int SourceSampleRate { get; set; }
        public int SampleRate { get; set; }
        public int Amplitude { get; set; }

        public ModelSetting(
            bool colored = true,
            int batchSize = 1,
            int windowSize = 256,
            int dftSize = 256,
            int hopSize = 3,
            int nMelBins = 1024,
            int sampleRate = 8192,
            int amplitude = 5000
            )
        {
            Colored = colored;
            BatchSize = batchSize;
            WindowSize = windowSize;
            DFTSize = dftSize;
            HopSize = hopSize;
            NMelBins = nMelBins;
            SourceSampleRate = sampleRate;
            SampleRate = sampleRate;
            Amplitude = amplitude;
        }
    }
}
