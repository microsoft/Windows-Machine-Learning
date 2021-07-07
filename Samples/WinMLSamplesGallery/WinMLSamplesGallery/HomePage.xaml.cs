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

// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.

namespace WinMLSamplesGallery
{
    public enum Model
    {
        AllSamples = 0,
        ImageClassifier,
        MobileNet,
        ResNet,
        SqueezeNet,
        VGG,
        AlexNet,
        GoogleNet,
        CaffeNet,
        RCNN_ILSVRC13,
        DenseNet121,
        Inception_V1,
        Inception_V2,
        ShuffleNet_V1,
        ShuffleNet_V2,
        ZFNet512,
        EfficientNetLite4,
    }

    public sealed class Link
    {
        public string Title { get; set; }
        public string Description { get; set; }
        public string Icon { get; set; }
        public Model Tag { get; set; }
    }

    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class HomePage : Page
    {
        public HomePage()
        {
            this.InitializeComponent();
        }

        private void StyledGrid_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            var gridView = sender as GridView;
            var link = gridView.SelectedItem as Link;
            var model = link.Tag;
            switch (model)
            {
                case Model.AllSamples:
                    // Only navigate if the selected page isn't currently loaded.
                    if (!Type.Equals(Frame.CurrentSourcePageType, typeof(AllSamples)))
                    {
                        Frame.Navigate(typeof(AllSamples), null, null);
                    }
                    break;
            }
        }
    }
}
