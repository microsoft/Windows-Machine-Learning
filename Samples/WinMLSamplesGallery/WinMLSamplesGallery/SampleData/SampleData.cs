using System;
using System.Threading.Tasks;
using Windows.Data.Json;
using Windows.Storage;

namespace WinMLSamplesGallery.SampleData
{
    public class SampleDataLoader
    {
        public JsonArray data;
        public SampleDataLoader() { }

        public async Task GetSampleData()
        {
            Uri dataUri = new Uri("ms-appx:///SampleData/SampleData.json");
            StorageFile file = await StorageFile.GetFileFromApplicationUriAsync(dataUri);
            string jsonText = await FileIO.ReadTextAsync(file);
            JsonObject jsonObject = JsonObject.Parse(jsonText);
            data = jsonObject["Samples"].GetArray();
        }
    }
}