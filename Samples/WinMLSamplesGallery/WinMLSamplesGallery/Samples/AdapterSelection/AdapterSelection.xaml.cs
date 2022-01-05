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

    public sealed partial class AdapterSelection : Page
    {
        public AdapterSelection()
        {
            this.InitializeComponent();
            System.Diagnostics.Debug.WriteLine("Initialized Adapter Selection");
            var some_str = WinMLSamplesGalleryNative.AdapterList.GetAdapters();
            System.Diagnostics.Debug.WriteLine("Called GetAdapters");
            System.Diagnostics.Debug.WriteLine(some_str);
        }
    }
}
