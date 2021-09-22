using System;
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
            StyledGrid.ItemsSource = await SampleMetadata.GetAllSampleMetadata();
        }

        private void NavigateToSample(object sender, SelectionChangedEventArgs e)
        {
            GridView gridView = sender as GridView;
            SampleMetadata sample = gridView.SelectedItem as SampleMetadata;
            
            // Only navigate if the selected page isn't currently loaded.
            if (!Type.Equals(MainWindow.mainFrame.CurrentSourcePageType, typeof(SampleBasePage)))
            {
                MainWindow.mainFrame.Navigate(typeof(SampleBasePage), sample);
            }
        }


    }
}

