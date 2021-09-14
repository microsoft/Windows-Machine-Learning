using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Navigation;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.Foundation;
using Windows.Foundation.Collections;
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

        public async void GetSampleData()
        {
            SampleDataList sampleDataList = new SampleDataList();
            JsonArray sampleData = await sampleDataList.GetSampleData();
            System.Diagnostics.Debug.WriteLine("Data {0}", sampleData);
        }

        protected override void OnNavigatedTo(NavigationEventArgs e)
        {
            pageInfo = (PageInfo) e.Parameter;
            System.Diagnostics.Debug.WriteLine("Navigated to this page title {0}, description {1}", pageInfo.PageTitle, pageInfo.PageDescription);

            SampleFrame.Navigate(typeof(Samples.Batching));
        }
    }
}
