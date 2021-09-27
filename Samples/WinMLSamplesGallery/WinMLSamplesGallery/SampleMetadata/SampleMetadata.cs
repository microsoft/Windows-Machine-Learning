using System;
using System.Collections.Generic;
using System.Threading.Tasks;
using Windows.Data.Json;
using Windows.Storage;

namespace WinMLSamplesGallery
{
    // Represents a link to documentation for a sample
    public sealed class SampleDoc
    {
        public string name { get; set; }
        public string link { get; set; }
    }

    // Represents the metadata for a sample used by the SampleBasePage
    public sealed class SampleMetadata
    {
        public string Title { get; set; }
        public string Description { get; set; }
        public string Icon { get; set; }
        public string Tag { get; set; }
        public string XAMLGithubLink { get; set; }
        public string CSharpGithubLink { get; set; }
        public List<SampleDoc> Docs { get; set; }

        // Grabs all the sample metadata from the SampleMetadata.json file
        // and returns it as a List<SampleMetadata>
        public static async Task<List<SampleMetadata>> GetAllSampleMetadata()
        {
            Uri fileUri = new Uri("ms-appx:///SampleMetadata/SampleMetadata.json");
            StorageFile file = await StorageFile.GetFileFromApplicationUriAsync(fileUri);
            string metadataJsonText = await FileIO.ReadTextAsync(file);
            JsonObject metadataJsonObject = JsonObject.Parse(metadataJsonText);
            JsonArray metadataJsonArray = metadataJsonObject["Samples"].GetArray();

            List<SampleMetadata> allSampleMetadata = new List<SampleMetadata>();
            for (int i = 0; i < metadataJsonArray.Count; i++)
            {
                JsonObject currentSampleMetadata = metadataJsonArray[i].GetObject();
                allSampleMetadata.Add(new SampleMetadata
                {
                    Title = currentSampleMetadata["Title"].GetString(),
                    Description = currentSampleMetadata["Description"].GetString(),
                    Icon = currentSampleMetadata["Icon"].GetString(),
                    Tag = currentSampleMetadata["Tag"].GetString(),
                    XAMLGithubLink = currentSampleMetadata["XAMLGithubLink"].GetString(),
                    CSharpGithubLink = currentSampleMetadata["CSharpGithubLink"].GetString(),
                    Docs = ConvertJsonArrayToSampleDocList(currentSampleMetadata["Docs"].GetArray())
                });
            }
            return allSampleMetadata;
        }

        private static List<SampleDoc> ConvertJsonArrayToSampleDocList(JsonArray arr)
        {
            List<SampleDoc> sampleDocList = new List<SampleDoc>();
            for (int i = 0; i < arr.Count; i++)
            {
                JsonObject jsonDoc = arr[i].GetObject();
                sampleDocList.Add(new SampleDoc
                {
                    name = jsonDoc["name"].GetString(),
                    link = jsonDoc["link"].GetString()
                });
            }
            return sampleDocList;
        }
    }
}
