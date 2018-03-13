using System;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using System.Threading.Tasks;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Media.Imaging;
using Windows.Storage;

using WinMLExplorer.ViewModels;

// The User Control item template is documented at https://go.microsoft.com/fwlink/?LinkId=234236

namespace WinMLExplorer.Controls
{
    public sealed partial class ImageControl : UserControl
    {
        public ImageControl()
        {
            this.InitializeComponent();
        }
        
        public void SetProgressIndicator(bool isActive)
        {
            this.progressIndicator.IsActive = isActive;
        }

        public async Task UpdateImageAsync(StorageFile imageFile)
        {
            if (imageFile == null)
            {
                return;
            }

            // Start progress indicator
            this.progressIndicator.IsActive = true;

            foreach (var child in this.hostGrid.Children.Where(c => !(c is Image)).ToArray())
            {
                this.hostGrid.Children.Remove(child);
            }

            // remove the current source
            this.bitmapImage.UriSource = null;

            try
            {
                this.bitmapImage.UriSource = new Uri(imageFile.Path);
                await EvaluateImageAsync(imageFile);

            }
            catch (Exception ex)
            {
                // If we fail to load the image we will just not display it
                this.bitmapImage.UriSource = null;
                Debug.WriteLine("Exception with UpdateImageAsync: " + ex);
            }

            // Stop progress indicator
            this.progressIndicator.IsActive = false;
        }

        private async Task EvaluateImageAsync(StorageFile imageFile)
        {
            MainViewModel viewModel = (MainViewModel)this.DataContext;

            if (viewModel == null || imageFile == null)
            {
                return;
            }

            await viewModel.EvaluateAsync(imageFile);
        }
    }
}
