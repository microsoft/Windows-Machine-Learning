using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace WinMLExplorer.Models
{
    public abstract class PropertyModel : INotifyPropertyChanged
    {

        public event PropertyChangedEventHandler PropertyChanged;

        protected SynchronizationContext SynchronizationContext;

        public PropertyModel()
            : this(SynchronizationContext.Current)
        {
        }

        public PropertyModel(SynchronizationContext synchronizationContext)
        {
            this.SynchronizationContext = synchronizationContext ?? throw new ArgumentNullException("Synchronization context is not initialized");
        }
        
        protected void RaisePropertyChanged(string prop)
        {
            if (this.PropertyChanged != null)
            {
                this.SynchronizationContext.Post(_ =>
                {
                    this.PropertyChanged(this, new PropertyChangedEventArgs(prop));
                }, null);
            }
        }
    }
}
