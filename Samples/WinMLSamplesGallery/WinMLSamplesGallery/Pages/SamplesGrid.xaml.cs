using System;
using System.Linq;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Navigation;

namespace WinMLSamplesGallery
{
    public sealed partial class SamplesGrid : Page
    {
        public SamplesGrid()
        {
            this.InitializeComponent();
        }

        protected override async void OnNavigatedTo(NavigationEventArgs e)
        {
            var allSamples = await SampleMetadata.GetAllSampleMetadata();
            if (e.Parameter == typeof(HomePage))
            {
                var recentlyAddedSamples = allSamples.Where(metadata => metadata.IsRecentlyAdded).Reverse();
                StyledGrid.ItemsSource = recentlyAddedSamples;
            }
            else
            {
                allSamples.Sort((left, right) => left.Title.CompareTo(right.Title));
                StyledGrid.ItemsSource = allSamples;
            }
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

