using System;
using System.Collections.Generic;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Windows.Storage;
using Windows.Graphics.Imaging;
using Windows.Media;
using Microsoft.AI.MachineLearning;
using Microsoft.AI.MachineLearning.Experimental;
using System.Threading.Tasks;
using System.Diagnostics;
using System.Linq;
using Windows.Storage.Streams;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.Foundation.Collections;
using Windows.Foundation;
using Microsoft.UI.Xaml.Media.Imaging;
using Windows.Devices.Enumeration;
using System.ComponentModel;
using System.Runtime.CompilerServices;
using Windows.Media.Capture;
using WinMLSamplesGallery.Controls;
using Windows.System.Display;

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
       
        public StreamEffect()
        {
            this.InitializeComponent();
            WinMLSamplesGalleryNative.StreamEffect.LaunchNewWindow();
        }



    }
}
