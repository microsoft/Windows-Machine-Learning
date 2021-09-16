using System;
using System.Collections.Generic;
using Microsoft.UI.Xaml.Controls;
using WinMLSamplesGallery.SampleData;
using Windows.Data.Json;

// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.

namespace WinMLSamplesGallery
{
    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class AllSamples : Page
    {
        public AllSamples()
        {
            this.InitializeComponent();
            SetSampleLinks();
        }

        private async void SetSampleLinks()
        {
            SampleDataLoader loader = new SampleDataLoader();
            await loader.GetSampleData();
            JsonArray sampleData = loader.data;

            List<Sample> samples = new List<Sample>();
            for (int i = 0; i < sampleData.Count; i++)
            {
                JsonObject sample = sampleData[i].GetObject();
                samples.Add(new Sample
                {
                    Title = sample["Title"].GetString(),
                    Description = sample["Description"].GetString(),
                    Icon = sample["Icon"].GetString(),
                    Tag = sample["Tag"].GetString(),
                    XAMLGithubLink = sample["XAMLGithubLink"].GetString(),
                    CSharpGithubLink = sample["CSharpGithubLink"].GetString(),
                    Docs = ConvertJsonArrayToSampleDocList(sample["Docs"].GetArray())
                });
            }
            StyledGrid.ItemsSource = samples;
        }

        private void NavigateToSample(object sender, SelectionChangedEventArgs e)
        {
            GridView gridView = sender as GridView;
            Sample sample = gridView.SelectedItem as Sample;

            // Only navigate if the selected page isn't currently loaded.
            if (!Type.Equals(Frame.CurrentSourcePageType, typeof(SampleBasePage)))
            {
                Frame.Navigate(typeof(SampleBasePage), sample);
            }
        }

        private List<SampleDoc> ConvertJsonArrayToSampleDocList(JsonArray arr)
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
