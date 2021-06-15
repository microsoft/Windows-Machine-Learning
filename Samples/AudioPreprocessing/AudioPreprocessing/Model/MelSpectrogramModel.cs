using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace AudioPreprocessing.Model
{
    public class MelSpectrogramModel
    {
        private List<string> repositoryData;
        public string ImportantInfo
        {
            get
            {
                return ConcatenateData(repositoryData);
            }
        }

        public MelSpectrogramModel()
        {
            repositoryData = GetData();
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
