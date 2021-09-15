using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Navigation;
using System;
using WinMLSamplesGallery.SampleData;
using Windows.Data.Json;

namespace WinMLSamplesGallery
{
    public sealed partial class SampleBasePage : Page
    {
        public PageInfo pageInfo;

        public SampleBasePage()
        {
            this.InitializeComponent();
        }

        private async System.Threading.Tasks.Task GetSampleData()
        {
            SampleDataLoader loader = new SampleDataLoader();
            await loader.GetSampleData();
            JsonArray sampleData = loader.data;
        }

        protected override async void OnNavigatedTo(NavigationEventArgs e)
        {
            await GetSampleData();
            pageInfo = (PageInfo) e.Parameter;
            System.Diagnostics.Debug.WriteLine("Navigated to this page title {0}, description {1}", pageInfo.PageTitle, pageInfo.PageDescription);

            SampleFrame.Navigate(typeof(Samples.Batching));
        }
    }
}
