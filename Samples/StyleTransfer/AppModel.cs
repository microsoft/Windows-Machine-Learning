using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Diagnostics;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices.WindowsRuntime;
using System.Text;
using System.Threading.Tasks;
using Windows.Media.Core;

namespace StyleTransfer
{
    class AppModel : INotifyPropertyChanged
    {
        private string _modelSource; // Not initialized vs. have a default model to use? 
        public string ModelSource
        {
            get { return _modelSource; }
            set
            {
                _modelSource = value;
                OnPropertyChanged();
            }
        }

        private string _inputMediaSource;
        public string InputMediaSource
        {
            get { return _inputMediaSource; }
            set
            {
                _inputMediaSource = value;
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

        public event PropertyChangedEventHandler PropertyChanged;
        private void OnPropertyChanged([CallerMemberName] string propertyName = null)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }

        public AppModel()
        {
            this._useGPU = true;
        }

    }
}
