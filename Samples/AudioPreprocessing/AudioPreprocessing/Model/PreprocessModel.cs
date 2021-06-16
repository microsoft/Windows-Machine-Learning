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
            MelSpecPath = "../tmp/mel_spectrogram_file.jpg";
        }
        public string MelSpecPath { get; set; }
    }
}
