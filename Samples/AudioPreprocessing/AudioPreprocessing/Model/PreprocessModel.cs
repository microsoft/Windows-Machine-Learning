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
        private List<string> repositoryData;
        private string melSpecPath;
        public string ImportantInfo
        {
            get
            {
                return ConcatenateData(repositoryData);
            }
        }
        public string MelSpecPath
        {
            get
            {
                return melSpecPath;
            }
        }

        public PreprocessModel()
        {
            repositoryData = GetData();
            melSpecPath = "C:/Users/t-janedu/source/repos/Windows-Machine-Learning/Samples/AudioPreprocessing/AudioPreprocessing/tmp/mel_spectrogram_file.jpg";
        }

        /// <summary>
        /// Simulates data retrieval from a repository
        /// </summary>
        /// <returns>List of strings</returns>
        private List<string> GetData()
        {
            repositoryData = new List<string>()
            {
                "Hello",
                "world"
            };
            return repositoryData;
        }

        /// <summary>
        /// Concatenate the information from the list into a fully formed sentence.
        /// </summary>
        /// <returns>A string</returns>
        private string ConcatenateData(List<string> dataList)
        {
            string importantInfo = dataList.ElementAt(0) + ", " + dataList.ElementAt(1) + "!";
            return importantInfo;
        }



    }
}
