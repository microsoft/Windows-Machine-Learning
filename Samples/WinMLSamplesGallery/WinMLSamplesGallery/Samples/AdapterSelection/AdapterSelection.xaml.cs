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

namespace WinMLSamplesGallery.Samples
{

    public sealed partial class AdapterSelection : Page
    {
        public AdapterSelection()
        {
            this.InitializeComponent();
            System.Diagnostics.Debug.WriteLine("Initialized Adapter Selection");
        }
    }
}
