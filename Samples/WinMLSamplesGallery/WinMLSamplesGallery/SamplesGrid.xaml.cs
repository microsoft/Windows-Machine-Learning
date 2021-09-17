using System;
using System.Collections.Generic;
using WinMLSamplesGallery.SampleData;
using Windows.Data.Json;
using Microsoft.UI.Xaml.Controls;

namespace WinMLSamplesGallery
{
    public sealed partial class SamplesGrid : Page
    {
        public SamplesGrid()
        {
            this.InitializeComponent();
            this.SetSampleLinks();
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
            if (!Type.Equals(MainWindow.mainFrame.CurrentSourcePageType, typeof(SampleBasePage)))
            {
                MainWindow.mainFrame.Navigate(typeof(SampleBasePage), sample);
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

