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
            sample = (Sample) e.Parameter;
            switch (sample.Tag)
            {
                case "ImageClassifier":
                    SampleFrame.Navigate(typeof(Samples.ImageClassifier));
                    break;
                case "Batching":
                    SampleFrame.Navigate(typeof(Samples.Batching));
                    break;
            }
        }
    }
}
