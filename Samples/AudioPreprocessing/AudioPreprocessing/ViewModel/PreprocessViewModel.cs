using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Input;
using AudioPreprocessing.Model;
using Microsoft.Toolkit.Mvvm;
using Microsoft.Toolkit.Mvvm.Input;
using MvvmDialogs;
using MvvmDialogs.FrameworkDialogs.OpenFile;

namespace AudioPreprocessing.ViewModel
{
    // Implements INotifyPropertyChanged interface to support bindings
    public class PreprocessViewModel : INotifyPropertyChanged
    {
        private readonly IDialogService dialogService;
        private string helloString;
        private string melSpecPath;

        public PreprocessViewModel()
        {
            PreprocessModel helloWorldModel = new PreprocessModel();
            melSpecPath = helloWorldModel.MelSpecPath;
            helloString = helloWorldModel.ImportantInfo;
            OpenFileCommand = new RelayCommand(OpenFile);
        }

        public string MelSpecPath
        {
            get { return melSpecPath; }
            set { melSpecPath = value; OnPropertyChanged(); }
        }

        public ICommand OpenFileCommand { get; }

        private void OpenFile()
        {
         
            var settings = new OpenFileDialogSettings
            {
                Title = "This Is The Title",
                //InitialDirectory = IOPath.GetDirectoryName(Assembly.GetExecutingAssembly().Location),
                Filter = "Text Documents (*.txt)|*.txt|All Files (*.*)|*.*"
            };

            bool? success = dialogService.ShowOpenFileDialog(this, settings);
            if (success == true)
            {
                melSpecPath = settings.FileName;
            }
        }

        public event PropertyChangedEventHandler PropertyChanged;

        public string HelloString
        {
            get { return helloString; }
            set { helloString = value; OnPropertyChanged(); }
        }


        /// <summary>
        /// Raises OnPropertychangedEvent when property changes
        /// </summary>
        /// <param name="name">String representing the property name</param>
        protected void OnPropertyChanged([CallerMemberName] string name = null)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(name));
        }

        
    }

}

