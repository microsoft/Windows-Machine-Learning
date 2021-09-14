using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Threading.Tasks;
using Windows.Data.Json;
using Windows.Storage;

namespace WinMLSamplesGallery.SampleData
{
    public class SampleDataList
    {
        public SampleDataList() { }

        public async JsonArray GetSampleData()
        {
            Uri dataUri = new Uri("ms-appx:///SampleData/SampleData.json");
            StorageFile file = await StorageFile.GetFileFromApplicationUriAsync(dataUri);
            string jsonText = await FileIO.ReadTextAsync(file);
            JsonObject jsonObject = JsonObject.Parse(jsonText);
            JsonArray data = jsonObject["Samples"].GetArray();
            return data;
        }
    }
}