using Microsoft.AI.MachineLearning;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Media;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Windows.Foundation.Metadata;
using Windows.Graphics.Imaging;
using Windows.Media;
using Windows.Storage;
using Windows.UI;
using WinMLSamplesGallery.Common;
using WinMLSamplesGallery.Controls;
using WinMLSamplesGalleryNative;

namespace WinMLSamplesGallery.Samples
{
    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class EncryptedResource : Page
    {
        public EncryptedResource()
        {
            this.InitializeComponent();
        }

        private void OnDecryptClick(object sender, RoutedEventArgs e)
        {

            // dummy call to load microsoft.ai.machinelearning... 
            var builder = Microsoft.AI.MachineLearning.Experimental.LearningModelBuilder.Create(11);
            // "WindowsMLSamples"
            var model = WinMLSamplesGalleryNative.EncryptedModels.LoadEncryptedResource(DecryptionKey.Password);

            if (model == null)
            {
                Fail.Visibility = Visibility.Visible;
                Succeed.Visibility = Visibility.Collapsed;
            }
            else
            {
                Fail.Visibility = Visibility.Collapsed;
                Succeed.Visibility = Visibility.Visible;
            }
        }
    }
}
