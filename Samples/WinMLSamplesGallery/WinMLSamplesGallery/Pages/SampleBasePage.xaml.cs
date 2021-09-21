using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Navigation;
using System;

namespace WinMLSamplesGallery
{
    public sealed partial class SampleBasePage : Page
    {
        public Sample sample;

        public SampleBasePage()
        {
            this.InitializeComponent();
        }

        protected override async void OnNavigatedTo(NavigationEventArgs e)
        {
            sample = (Sample)e.Parameter;
            switch (sample.Tag)
            {
                case "ImageClassifier":
                    SampleFrame.Navigate(typeof(Samples.ImageClassifier));
                    break;
                case "Batching":
                    SampleFrame.Navigate(typeof(Samples.Batching));
                    break;
            }
            if (sample.Docs.Count > 0)
                DocsHeader.Visibility = Visibility.Visible;
            else
                DocsHeader.Visibility = Visibility.Collapsed;
        }

        protected override async void OnNavigatingFrom(NavigatingCancelEventArgs e)
        {
            // Batching Sample contains background thread events that may need to be stopped.
            if (SampleFrame.SourcePageType == typeof(Samples.Batching))
            {
                var page = (Samples.Batching)SampleFrame.Content;
                page.StopAllEvents();
            }
        }
    }
}
