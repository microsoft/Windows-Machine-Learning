using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Input;
using Windows.Media.Core;

namespace StyleTransfer
{
    class AppViewModel : INotifyPropertyChanged
    {
        private AppModel _appModel;
        public AppModel CurrentApp
        {
            get { return _appModel; }
        }

        private ICommand _saveCommand;
        public ICommand SaveCommand
        {
            get { return _saveCommand; }
            set
            {
                _saveCommand = value;
            }
        }

        public ICommand MediaSourceCommand { get; set; }

        public event PropertyChangedEventHandler PropertyChanged;
        public AppViewModel()
        {
            _appModel = new AppModel();
            SaveCommand = new RelayCommand(new Action<object>(SaveOutput));
            MediaSourceCommand = new RelayCommand(new Action<object>(SetMediaSource));
        }

        public void SaveOutput(object obj)
        {
            return;
        }
        public void SetMediaSource(object obj)
        {
            _appModel.InputMediaSource = obj.ToString();
            return;
        }
    }
}
