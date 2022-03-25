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
            WinMLSamplesGalleryNative.DXResourceBinding.BindDXResourcesUsingORT();
        }
    }
}
