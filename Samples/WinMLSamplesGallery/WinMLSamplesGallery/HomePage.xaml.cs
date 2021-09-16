using Microsoft.UI.Xaml.Controls;
using System;
using System.Collections.Generic;
using WinMLSamplesGallery.SampleData;
using Windows.Data.Json;

// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.

namespace WinMLSamplesGallery
{
    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class HomePage : Page
    {
        public HomePage()
        {
            this.InitializeComponent();
            SamplesGrid.Navigate(typeof(SamplesGrid));
        }
    }
}
