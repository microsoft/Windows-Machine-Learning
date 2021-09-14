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

namespace WinMLSamplesGallery
{
    public sealed partial class SampleBasePage : Page
    {
        public PageInfo pageInfo;

        public SampleBasePage()
        {
            this.InitializeComponent();
        }

        protected override void OnNavigatedTo(NavigationEventArgs e)
        {
            pageInfo = (PageInfo) e.Parameter;
            System.Diagnostics.Debug.WriteLine("Navigated to this page title {0}, description {1}", pageInfo.PageTitle, pageInfo.PageDescription);

            this.SampleFrame.Navigate(typeof(Samples.Batching));
        }
    }
}
