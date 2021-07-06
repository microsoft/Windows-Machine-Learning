using System;
using System.Collections.Generic;
using System.Text;

namespace Model
{
    public class ModelSetting
    {
        public int BatchSize { get; set; }
        public int WindowSize { get; set; }
        public int DFTSize { get; set; }
        public int HopSize { get; set; }
        public int NMelBins { get; set; }
        public int SampleRate { get; set; }
        public int Amplitude { get; set; }

        ModelSetting()
        {
            BatchSize = 1;
            WindowSize = 256;
            DFTSize = 256;
            HopSize = 3;
            NMelBins = 1024;
            SampleRate = 8192;
            Amplitude = 5000;
        }
    }
}
