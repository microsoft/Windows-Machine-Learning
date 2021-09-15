using Microsoft.UI.Xaml.Controls;
using System;
using System.Collections.Generic;
using WinMLSamplesGallery.SampleData;
using Windows.Data.Json;

// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.

namespace WinMLSamplesGallery
{
    public sealed class Sample
    {
        public string Title { get; set; }
        public string Description { get; set; }
        public string Icon { get; set; }
        public string Tag { get; set; }
        public string XAMLGithubLink { get; set; }
        public string CSharpGithubLink { get; set; }
        public string DocsLink { get; set; }
    }

    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class HomePage : Page
    {
        public HomePage()
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
                    DocsLink = sample["DocsLink"].GetString()
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
    }
}
