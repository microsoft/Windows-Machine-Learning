using Microsoft.UI.Xaml.Controls;

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

        private void Button_Click(object sender, Microsoft.UI.Xaml.RoutedEventArgs e)
        {
           WinMLSamplesGalleryNative.Class.MessageBoxFromWin32();
        }
    }
}
