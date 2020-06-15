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
using Windows.Media.Core;
using Windows.Media.Playback;

namespace StyleTransfer
{
    class AppModel : INotifyPropertyChanged
    {
        public AppModel()
        {
            this._useGPU = true;
            this._selectedCameraIndex = 0;
            this._modelSource = "candy";
        }

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

        private string _inputMedia;
        public string InputMedia
        {
            get { return _inputMedia; }
            set
            {
                _inputMedia = value;
                OnPropertyChanged();
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

        private MediaSource _outputMediaSource;
        public MediaSource OutputMediaSource
        {
            get
            {
                // TODO: Configure mediasource based on profile? or in VM
                return _outputMediaSource;
            }
            set { _outputMediaSource = value; OnPropertyChanged(); }
        }
        private MediaSource _inputMediaSource;
        public MediaSource InputMediaSource
        {
            get { return _inputMediaSource; }
            set { _inputMediaSource = value; OnPropertyChanged(); }
        }

        private IEnumerable<string> _cameraNamesList;
        public IEnumerable<string> CameraNamesList
        {
            get { return _cameraNamesList; }
            set { _cameraNamesList = value; OnPropertyChanged(); }
        }

        private int _selectedCameraIndex;
        public int SelectedCameraIndex
        {
            get { return _selectedCameraIndex; }
            set { _selectedCameraIndex = value; OnPropertyChanged(); }
        }

        public event PropertyChangedEventHandler PropertyChanged;
        private void OnPropertyChanged([CallerMemberName] string propertyName = null)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }
    }
}
