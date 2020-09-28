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
using Windows.UI.Xaml.Controls;

namespace StyleTransfer
{
    enum InputMediaType
    {
        None,
        LiveStream,
        AcquireImage,
        FilePick,
        VideoPick
    }
    class AppModel : INotifyPropertyChanged
    {
        public AppModel()
        {
            this._modelSource = "candy";
            this._selectedCameraIndex = 0;
            this._outputCaptureElement = new CaptureElement();
        }

        public List<String> ModelFileNames
        {
            get
            {
                return new List<string>
                {   "candy",
                    "mosaic",
                    "la_muse",
                    "udnie"
                };
            }
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
        private InputMediaType _inputMedia;
        public InputMediaType InputMedia
        {
            get { return _inputMedia; }
            set
            {
                _inputMedia = value;
                OnPropertyChanged();
                OnPropertyChanged("StreamingControlsVisible");
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
        private CaptureElement _outputCaptureElement;
        public CaptureElement OutputCaptureElement
        {
            get
            {
                if (_outputCaptureElement == null)
                    _outputCaptureElement = new CaptureElement();
                return _outputCaptureElement;
            }
            set
            {
                _outputCaptureElement = value;
                OnPropertyChanged();
            }
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
                return _inputMedia == InputMediaType.LiveStream;
            }
        }

        public event PropertyChangedEventHandler PropertyChanged;
        private void OnPropertyChanged([CallerMemberName] string propertyName = null)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }
    }
}
