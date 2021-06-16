using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Input;
using AudioPreprocessing.Model;


namespace AudioPreprocessing.ViewModel
{
    // Implements INotifyPropertyChanged interface to support bindings
    public class PreprocessViewModel : INotifyPropertyChanged
    
    {
        private string melSpecPath;

        public PreprocessViewModel()
        {
            PreprocessModel helloWorldModel = new PreprocessModel();
            melSpecPath = helloWorldModel.MelSpecPath;
        }

        public string MelSpecPath
        {
            get { return melSpecPath; }
            set { melSpecPath = value; OnPropertyChanged(); }
        }

        public ICommand OpenFileCommand { get; }

        private void OpenFile()
        {
            //Logic for opening files
            Console.WriteLine("Open File");
        }

        public event PropertyChangedEventHandler PropertyChanged;

        protected void OnPropertyChanged([CallerMemberName] string name = null)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(name));
        }

        
    }

}

