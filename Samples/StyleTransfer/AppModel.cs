using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Diagnostics;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices.WindowsRuntime;
using System.Text;
using System.Threading.Tasks;
using Windows.Media;
using Windows.Media.Capture;
using Windows.Media.Core;
using Windows.Media.Playback;
using Windows.UI.Composition;

namespace StyleTransfer
{
    class AppModel : INotifyPropertyChanged
    {
        public AppModel()
        {
            this._useGPU = true;
            this._modelSource = "candy";
        }

        // Name of the style transfer model to apply to input media
        private string _modelSource;
        public string ModelSource
        {
            get { return _modelSource; }
            set
            {
                _modelSource = value;
                OnPropertyChanged();
            }
        }

        // Type of input media to use
        private string _inputMedia;
        public string InputMedia
        {
            get { return _inputMedia; }
            set
            {
                _inputMedia = value;
                OnPropertyChanged();
                OnPropertyChanged("StreamingControlsVisible");
            }
        }

        private bool _useGPU;
        public bool UseGPU
        {
            get { return _useGPU; }
            set
            {
                _useGPU = value;
                OnPropertyChanged();
            }
        }

        // Input VideoFrame when processing images
        private VideoFrame _inputFrame;
        public VideoFrame InputFrame
        {
            get { return _inputFrame; }
            set
            {
                _inputFrame = value;
                OnPropertyChanged();
            }
        }

        // Output VideoFrame when processing images
        private VideoFrame _outputFrame;
        public VideoFrame OutputFrame
        {
            get { return _outputFrame; }
            set
            {
                _outputFrame = value;
                OnPropertyChanged();
            }
        }
        // MediaSource for transformed media
        private MediaSource _outputMediaSource;
        public MediaSource OutputMediaSource
        {
            get { return _outputMediaSource; }
            set { _outputMediaSource = value; OnPropertyChanged(); }
        }

        // MediaSource for the input media
        private MediaSource _inputMediaSource;
        public MediaSource InputMediaSource
        {
            get { return _inputMediaSource; }
            set { _inputMediaSource = value; OnPropertyChanged(); }
        }

        // List of cameras available to use as input
        private IEnumerable<string> _cameraNamesList;
        public IEnumerable<string> CameraNamesList
        {
            get { return _cameraNamesList; }
            set { _cameraNamesList = value; OnPropertyChanged(); }
        }

        // Index in CameraNamesList of the selected input camera
        private int _selectedCameraIndex;
        public int SelectedCameraIndex
        {
            get { return _selectedCameraIndex; }
            set
            {
                _selectedCameraIndex = value;
                OnPropertyChanged();
            }
        }

        public bool StreamingControlsVisible
        {
            get
            {
                return _inputMedia == "LiveStream";
            }
        }


        public event PropertyChangedEventHandler PropertyChanged;
        private void OnPropertyChanged([CallerMemberName] string propertyName = null)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }
    }
}
