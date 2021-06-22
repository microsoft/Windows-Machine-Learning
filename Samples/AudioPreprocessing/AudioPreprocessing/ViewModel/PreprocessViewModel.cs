using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Input;
using System.Windows.Media;
using AudioPreprocessing.Model;
using Microsoft.Win32;

namespace AudioPreprocessing.ViewModel
{
    // Implements INotifyPropertyChanged interface to support bindings
    public class PreprocessViewModel : INotifyPropertyChanged

    {
        private string audioPath;
        private string imagePath;
        private ImageSource melSpectrogramImage;

        public PreprocessViewModel()
        {
            PreprocessModel preprocessModel = new PreprocessModel();
            audioPath = preprocessModel.AudioPath;
            imagePath = preprocessModel.MelSpecImagePath;
            melSpectrogramImage = null;
        }

        public string AudioPath
        {
            get { return audioPath; }
            set { audioPath = value; OnPropertyChanged(); }
        }
        public string ImagePath
        {
            get { return imagePath; }
            set { imagePath = value; OnPropertyChanged(); }
        }

        public ImageSource MelSpectrogramImage
        {
            get { return melSpectrogramImage; }
            set { melSpectrogramImage = value; OnPropertyChanged(); }
        }

        public ICommand OpenFileCommand => new RelayCommand(OpenFile);

        private void OpenFile(object commandParameter)
        {
            var openFileDialog = new OpenFileDialog();
            openFileDialog.Title = "Choose an Audio File";
            openFileDialog.Filter = "sound files (*.wav)|*.wav|All files (*.*)|*.*";

            if (openFileDialog.ShowDialog() == true)
            {
                PreprocessModel melSpec = new PreprocessModel();
                AudioPath = openFileDialog.FileName;
                //ImagePath = melSpec.GenerateMelSpectrogram(AudioPath);
                MelSpectrogramImage = melSpec.GenerateMelSpectrogram(AudioPath);
            }
        }

        public event PropertyChangedEventHandler PropertyChanged;

        protected void OnPropertyChanged([CallerMemberName] string name = null)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(name));
        }


    }

}

