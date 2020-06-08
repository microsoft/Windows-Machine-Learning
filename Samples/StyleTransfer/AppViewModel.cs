using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Input;
using Windows.Media.Core;
using GalaSoft.MvvmLight.Command;

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
            // TODO: Add a param to check if have an image to save
            SaveCommand = new RelayCommand(this.SaveOutput);
            MediaSourceCommand = new RelayCommand<string>(SetMediaSource);
        }

        public void SaveOutput()
        {
            return;
        }
        public void SetMediaSource(object obj)
        {
            // TODO: Convert to a better value for the appModel object here 
            _appModel.InputMediaSource = obj.ToString();
            return;
        }
    }
}
