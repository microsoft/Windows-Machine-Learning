using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Navigation;

// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.

namespace WinMLSamplesGallery
{
    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class AllSamples : Page
    {
        public AllSamples()
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
                case PageId.Batching:
                    // Only navigate if the selected page isn't currently loaded.
                    if (!Type.Equals(Frame.CurrentSourcePageType, typeof(Samples.Batching)))
                    {
                        Frame.Navigate(typeof(Samples.Batching), null, null);
                    }
                    break;
            }
        }
    }
}
