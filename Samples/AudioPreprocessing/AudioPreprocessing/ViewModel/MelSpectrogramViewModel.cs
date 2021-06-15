using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Text;
using System.Threading.Tasks;
using AudioPreprocessing.Model;

namespace AudioPreprocessing.ViewModel
{
    // Implements INotifyPropertyChanged interface to support bindings
    public class MelSpectrogramViewModel : INotifyPropertyChanged
    {
        private string helloString;

        public event PropertyChangedEventHandler PropertyChanged;

        public string HelloString
        {
            get
            {
                return helloString;
            }
            set
            {
                helloString = value;
                OnPropertyChanged();
            }
        }

        /// <summary>
        /// Raises OnPropertychangedEvent when property changes
        /// </summary>
        /// <param name="name">String representing the property name</param>
        protected void OnPropertyChanged([CallerMemberName] string name = null)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(name));
        }

        public MelSpectrogramViewModel()
        {
            MelSpectrogramModel helloWorldModel = new MelSpectrogramModel();
            helloString = helloWorldModel.ImportantInfo;
        }
    }

}

