using Microsoft.AI.MachineLearning;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using System;
using System.Collections.Generic;
using System.IO;
using System.Threading.Tasks;
using Windows.Graphics.Imaging;
using Windows.Media;
using Windows.Storage;
using WinMLSamplesGalleryNative;

namespace WinMLSamplesGallery.Samples
{
    public sealed partial class DXResourceBindingORT : Page
    {
        public DXResourceBindingORT()
        {
            this.InitializeComponent();
            //WinMLSamplesGalleryNative.DXResourceBinding.BindDXResourcesUsingORT();
        }

        private void LaunchWindow(object sender, RoutedEventArgs e)
        {
            Task.Run(() => WinMLSamplesGalleryNative.DXResourceBinding.LaunchWindow());
            //WinMLSamplesGalleryNative.DXResourceBinding.EvalORT();
            System.Threading.Thread.Sleep(1000);
            float[] results = WinMLSamplesGalleryNative.DXResourceBinding.EvalORT();
            var results_lst = new List<float>(results);
            System.Diagnostics.Debug.WriteLine("In C# code");
            for(int i = 0; i < results_lst.Count; i++)
            {
                System.Diagnostics.Debug.WriteLine(i.ToString() + ": " + results_lst[i].ToString());
            }
        }
    }
}
