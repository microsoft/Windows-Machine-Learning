using System;
using System.Collections.Generic;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Windows.Storage;

using System.Threading.Tasks;
using System.Diagnostics;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;

using Windows.Devices.Enumeration;
using System.ComponentModel;
using System.Runtime.CompilerServices;
using System.IO;


using Windows.Media.Editing;
using Windows.Media.Core;
using Windows.Media.Playback;

using WinMLSamplesGalleryNative;


namespace WinMLSamplesGallery.Samples
{
    public class StreamEffectViewModel : INotifyPropertyChanged
    {
        private int _selectedCameraIndex;
        public int SelectedCameraIndex 
        {
            get { return _selectedCameraIndex; }
            set {
                _selectedCameraIndex = value;
                Debug.WriteLine($"Update camera index to:{_selectedCameraIndex}");
                OnPropertyChanged();
            } 
        }

        public IEnumerable<string> CameraNamesList { get; set; }
        public DeviceInformationCollection devices;
        public event PropertyChangedEventHandler PropertyChanged;
        protected virtual void OnPropertyChanged([CallerMemberName] string propertyName = null)
        {
            Debug.WriteLine($"Update property {propertyName}");
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }

        public async void GetDevices()
        {
            if (devices == null)
            {
                try
                {
                    devices = await DeviceInformation.FindAllAsync(DeviceClass.VideoCapture);
                }
                catch (Exception ex)
                {
                    Debug.WriteLine(ex.Message);
                    devices = null;
                }
                if ((devices == null) || (devices.Count == 0))
                {
                    // No camera sources found
                    Debug.WriteLine("No cameras found");
                    //NotifyUser(false, "No cameras found.");
                    return;
                }
                CameraNamesList = devices.Select(device => device.Name);
                Debug.WriteLine($"Devices: {string.Join(", ", CameraNamesList.ToArray<string>())}");

                return;
            }
        }
    }

    public sealed partial class StreamEffect : Page
    {
        string modelPath;
        bool isPreviewing = false;
       
        public StreamEffect()
        {
            this.InitializeComponent();
            // TODO: Currently the WinMLSamplesGalleryNative component will load
            // the wrong version of the Microsoft.AI.MachineLearning.dll.
            // To work around this, we make a dummy call to the builder to
            // ensure that the dll is loaded.
            var builder = Microsoft.AI.MachineLearning.Experimental.LearningModelBuilder.Create(11);

            //var modelName = "mosaic.onnx";
            modelPath = Path.Join(Windows.ApplicationModel.Package.Current.InstalledLocation.Path, "Models");
            
        }

        // TODO: If keep in a separate window, send a task to shut down all stream effect processes
        public void ShutDownWindow() 
        {
            return; 
        }
        async private void ToggleInference(object sender, RoutedEventArgs e)
        {
            //isPreviewing = true;
            isPreviewing = !isPreviewing; // Toggle the previewing bool

            if (isPreviewing)
            {
                // Change the button text/symbol on the button to prompt user to close window on next click
                //ToggleInferenceBtn.Content = (StackPanel) this.Resources["StopInference"];
                ToggleInferenceBtnText.Text = "Stop Inference";
                ToggleInferenceBtnIcon.Symbol = Symbol.Cancel;

                // Running as a task ensures that the samples gallery window isn't blocked.
                Task.Run(() => WinMLSamplesGalleryNative.StreamEffect.LaunchNewWindow(modelPath));
            }
            else if(!isPreviewing)
            {
                ToggleInferenceBtnText.Text = "Start Inference";
                ToggleInferenceBtnIcon.Symbol = Symbol.NewWindow;

                // TODO: Implement the ShutDownWindow function
            }
            
        }

    }
}
