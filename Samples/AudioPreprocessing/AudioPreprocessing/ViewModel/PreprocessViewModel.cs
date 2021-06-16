using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Input;
using AudioPreprocessing.Model;
using Microsoft.Win32;

namespace AudioPreprocessing.ViewModel
{
    // Implements INotifyPropertyChanged interface to support bindings
    public class PreprocessViewModel : INotifyPropertyChanged

    {
        private string melSpecPath;

        public PreprocessViewModel()
        {
            PreprocessModel preprocessModel = new PreprocessModel();
            melSpecPath = preprocessModel.AudioPath;
        }

        public string MelSpecPath
        {
            get { return melSpecPath; }
            set { melSpecPath = value; OnPropertyChanged(); }
        }

        public ICommand OpenFileCommand => new RelayCommand(OpenFile);

        private void OpenFile(object commandParameter)
        {
            var openFileDialog = new OpenFileDialog();
            openFileDialog.Title = "Choose an Audio File";
            openFileDialog.Filter = "sound files (*.wav)|*.wav|All files (*.*)|*.*";

            if (openFileDialog.ShowDialog() == true)
            {
                MelSpecPath = openFileDialog.FileName;
            }
        }

        public event PropertyChangedEventHandler PropertyChanged;

        protected void OnPropertyChanged([CallerMemberName] string name = null)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(name));
        }


    }

}

