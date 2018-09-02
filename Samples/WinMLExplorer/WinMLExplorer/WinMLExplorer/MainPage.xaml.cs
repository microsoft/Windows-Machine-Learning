using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading.Tasks;
using Windows.Storage;
using Windows.UI;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;

using WinMLExplorer.MLModels;
using WinMLExplorer.ViewModels;

namespace WinMLExplorer
{
    /// <summary>
    /// The main page of the application
    /// </summary>
    public sealed partial class MainPage : Page
    {
        /// <summary>
        /// View model that represents this main page
        /// </summary>
        public MainViewModel ViewModel { get; set; }

        /// <summary>
        /// Constructor for the MainPage
        /// </summary>
        public MainPage()
        {
            this.ViewModel = new MainViewModel();

            this.InitializeComponent();

            // Set the model selection combo box to the current model
            if (this.ViewModel.CurrentModel != null)
            {
                this.modelComboBox.SelectedItem = this.ViewModel.CurrentModel;
            }

            // Set the camera combo box to the first camera on the device
            if (this.ViewModel.CameraNames.Count() > 0)
            {
                this.cameraSourceComboBox.SelectedItem = this.ViewModel.CameraNames[0];
            }
        }

        /// <summary>
        /// Event handler for camera source changes
        /// </summary>
        private async void OnCameraSourceSelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            await this.RestartWebCameraAsync();
        }

        /// <summary>
        /// Event handler for preferred device toggle changes
        /// </summary>
        private async void OnDeviceToggleToggled(object sender, RoutedEventArgs e)
        {
            await this.ViewModel.CurrentModel.SetIsGpuValue(this.deviceToggle.IsOn);
        }

        /// <summary>
        /// Event handler for camera source changes
        /// </summary>
        private async void OnModelSelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            // Clear existing results
            this.ViewModel.Results.Clear();

            this.ViewModel.CurrentModel = (WinMLModel)this.modelComboBox.SelectedItem;

            if (this.cameraControl.CameraStreamState == Windows.Media.Devices.CameraStreamState.Streaming)
            {
                await this.RestartWebCameraAsync();
            }
            else
            {
                // Hide UI elements
                this.durationTextBlock.Visibility = Visibility.Collapsed;
                this.cameraSource.Visibility = Visibility.Collapsed;
                this.resultsViewer.Visibility = Visibility.Collapsed;
                this.webCameraHostGrid.Visibility = Visibility.Collapsed;
                this.imageHostGrid.Visibility = Visibility.Collapsed;

                // Show UI elements
                this.landingMessage.Visibility = Visibility.Visible;
            }
        }

        /// <summary>
        /// Event handler for image selection changes
        /// </summary>
        private async void OnImagePickerSelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            StorageFile selectedImageFile = (StorageFile)this.imagePickerGridView.SelectedValue;

            this.imagePickerFlyout.Hide();

            if (selectedImageFile != null)
            {
                // Stop video stream
                await this.cameraControl.StopStreamAsync();

                // Hide UI elements
                this.durationTextBlock.Visibility = Visibility.Collapsed;
                this.cameraSource.Visibility = Visibility.Collapsed;
                this.landingMessage.Visibility = Visibility.Collapsed;
                this.resultsViewer.Visibility = Visibility.Collapsed;
                this.webCameraHostGrid.Visibility = Visibility.Collapsed;

                // Process images
                this.imageHostGrid.Visibility = Visibility.Visible;
                await this.imageControl.UpdateImageAsync(selectedImageFile);

                // Show UI elements
                this.durationTextBlock.Visibility = Visibility.Visible;
                this.resultsViewer.Visibility = Visibility.Visible;
            }
        }

        /// <summary>
        /// Event handler when the page resizes
        /// </summary>
        private void OnPageSizeChanged(object sender, SizeChangedEventArgs e)
        {
            UpdateWebCameraHostGridSize();
        }

        /// <summary>
        /// Event handler for when web camera button is clicked
        /// </summary>
        private async void OnWebCameraButtonClicked(object sender, RoutedEventArgs e)
        {
            await StartWebCameraAsync();
        }

        /// <summary>
        /// Restart the web camera
        /// </summary>
        private async Task RestartWebCameraAsync()
        {
            if (this.cameraSourceComboBox.SelectedItem == null)
            {
                return;
            }

            if (this.cameraControl.CameraStreamState == Windows.Media.Devices.CameraStreamState.Streaming)
            {
                await this.cameraControl.StopStreamAsync();
                await Task.Delay(1000);
                await this.cameraControl.StartStreamAsync(this.ViewModel, this.cameraSourceComboBox.SelectedItem.ToString());
            }
        }

        /// <summary>
        /// Event handler for camera source changes
        /// </summary>
        private async Task StartWebCameraAsync()
        {
            if (this.cameraSourceComboBox.SelectedItem == null)
            {
                return;
            }

            // Hide UI elements
            this.durationTextBlock.Visibility = Visibility.Collapsed;
            this.imageHostGrid.Visibility = Visibility.Collapsed;
            this.landingMessage.Visibility = Visibility.Collapsed;
            this.resultsViewer.Visibility = Visibility.Collapsed;

            // Start camera
            this.webCameraHostGrid.Visibility = Visibility.Visible;
            await this.cameraControl.StartStreamAsync(this.ViewModel, this.cameraSourceComboBox.SelectedItem.ToString());
            await Task.Delay(250);

            // Show UI elements
            this.cameraSource.Visibility = Visibility.Visible;
            this.durationTextBlock.Visibility = Visibility.Visible;
            this.resultsViewer.Visibility = Visibility.Visible;

            UpdateWebCameraHostGridSize();
        }

        /// <summary>
        /// Update the web camera host grid size
        /// </summary>
        private void UpdateWebCameraHostGridSize()
        {
            this.webCameraHostGrid.Height = this.webCameraHostGrid.ActualWidth / (this.cameraControl.CameraAspectRatio != 0 ? this.cameraControl.CameraAspectRatio : 1.777777777777);
        }
    }
}
