using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Input;

namespace VideoStreamDemo
{

    public class PageViewModel : INotifyPropertyChanged
    {

        public List<string> modelFileNames = new List<string>
            {"candy",
                "mosaic",
                "la_muse",
                "udnie"
            };

        public event PropertyChangedEventHandler PropertyChanged;
        public PageViewModel()
        {
            useGpu = true;
            selectedModel = "candy";
        }

        bool useGpu; // Determines if should use GPU. Default to True.
        public bool InferenceDevice
        {
            set
            {
                if (useGpu != value)
                {
                    useGpu = value;
                    OnPropertyChanged("InferenceDevice");
                }
            }
            get
            {
                return useGpu;
            }
        }

        private string selectedModel;
        public string SelectedModel
        {
            get { return selectedModel; }
            set
            {
                if (value != this.selectedModel)
                {
                    selectedModel = value;
                    OnPropertyChanged("SelectedModel");
                }
            }
        }

        private string selectedSource;
        public string SelectedSource { get; set; }

        // Trying to figure out how to send command for selecting media source
        /*public ICommand SelectSourceCommand
        {
            get
            {
                return new Command<string>((x) => selectedSource = x);
            }
        }*/


        private void OnPropertyChanged(string propertyName)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }
    }
}
