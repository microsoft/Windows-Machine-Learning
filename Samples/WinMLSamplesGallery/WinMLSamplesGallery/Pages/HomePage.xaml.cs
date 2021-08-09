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
    public enum PageId
    {
        AllSamples = 0,
        ImageClassifier,
        ImageEffects,
        ObjectDetector,
    }

    public sealed class Link
    {
        public string Title { get; set; }
        public string Description { get; set; }
        public string Icon { get; set; }
        public PageId Tag { get; set; }
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
                case PageId.ImageClassifier:
                    // Only navigate if the selected page isn't currently loaded.
                    if (!Type.Equals(Frame.CurrentSourcePageType, typeof(Samples.ImageClassifier)))
                    {
                        Frame.Navigate(typeof(Samples.ImageClassifier), null, null);
                    }
                    break;
                case PageId.ImageEffects:
                    // Only navigate if the selected page isn't currently loaded.
                    if (!Type.Equals(Frame.CurrentSourcePageType, typeof(Samples.ImageEffects)))
                    {
                        Frame.Navigate(typeof(Samples.ImageEffects), null, null);
                    }
                    break;
                case PageId.ObjectDetector:
                    // Only navigate if the selected page isn't currently loaded.
                    if (!Type.Equals(Frame.CurrentSourcePageType, typeof(Samples.ObjectDetector)))
                    {
                        Frame.Navigate(typeof(Samples.ObjectDetector), null, null);
                    }
                    break;
            }
        }
    }
}
