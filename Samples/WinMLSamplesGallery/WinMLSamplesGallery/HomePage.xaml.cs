﻿using Microsoft.UI.Xaml.Controls;
using System;
using System.Collections.Generic;
using WinMLSamplesGallery.SampleData;
using Windows.Data.Json;

// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.

namespace WinMLSamplesGallery
{
    public enum PageId
    {
        AllSamples = 0,
        ImageClassifier,
        ImageEffects,
        Batching,
        SampleBasePage
    }

    public sealed class Link
    {
        public string Title { get; set; }
        public string Description { get; set; }
        public string Icon { get; set; }
        public PageId Tag { get; set; }
    }

    public sealed class PageInfo
    {
        public string PageTitle { get; set; }
        public string PageDescription { get; set; }
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

            List<Link> links = new List<Link>();
            for (int i = 0; i < sampleData.Count; i++)
            {
                JsonObject sample = sampleData[i].GetObject();
                PageId sampleTag = GetTagFromString(sample["Tag"].GetString());
                links.Add(new Link
                {
                    Title = sample["Title"].GetString(),
                    Description = sample["Description"].GetString(),
                    Icon = sample["Icon"].GetString(),
                    Tag = sampleTag
                });
            }
            StyledGrid.ItemsSource = links;
        }

        private PageId GetTagFromString(string str)
        {
            PageId id = PageId.AllSamples;
            if (str == "ImageClassifier")
                id = PageId.ImageClassifier;
            else if (str == "Batching")
                id = PageId.Batching;
            return id;
        }

        private void StyledGrid_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            var gridView = sender as GridView;
            var link = gridView.SelectedItem as Link;
            var model = link.Tag;
            switch (model)
            {
                case PageId.ImageClassifier:
                    // Only navigate if the selected page isn't currently loaded.
                    if (!Type.Equals(Frame.CurrentSourcePageType, typeof(Samples.ImageClassifier)))
                    {
                        Frame.Navigate(typeof(Samples.ImageClassifier), null, null);
                    }
                    break;
                case PageId.ImageEffects:
                    // Only navigate if the selected page isn't currently loaded.
                    if (!Type.Equals(Frame.CurrentSourcePageType, typeof(Samples.ImageEffects)))
                    {
                        Frame.Navigate(typeof(Samples.ImageEffects), null, null);
                    }
                    break;
                case PageId.Batching:
                    // Only navigate if the selected page isn't currently loaded.
                    if (!Type.Equals(Frame.CurrentSourcePageType, typeof(Samples.Batching)))
                    {
                        Frame.Navigate(typeof(Samples.Batching), null, null);
                    }
                    break;
                case PageId.SampleBasePage:
                    // Only navigate if the selected page isn't currently loaded.
                    if (!Type.Equals(Frame.CurrentSourcePageType, typeof(SampleBasePage)))
                    {
                        Frame.Navigate(typeof(SampleBasePage), new PageInfo {PageTitle="Test Title", PageDescription="Test Description"});
                    }
                    break;
            }
        }
    }
}
