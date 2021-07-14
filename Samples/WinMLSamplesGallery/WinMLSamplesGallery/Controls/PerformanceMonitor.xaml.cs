using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Navigation;
using System.Collections.ObjectModel;

// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.

namespace WinMLSamplesGallery.Controls
{
    public sealed class Metric
    {
        public string Title { get; set; }
        public string Duration { get; set; }
    }

    public sealed partial class PerformanceMonitor : UserControl
    {
        private ObservableCollection<Metric> Items { get; } = new ObservableCollection<Metric>();

        public PerformanceMonitor()
        {
            this.InitializeComponent();

            this.Visibility = Visibility.Collapsed;
        }

        public void Log(string title, float duration) {
            var duationString = String.Format("{0:00.0000}", duration);
            Items.Add(new Metric { Title = title, Duration = duationString });
            this.Visibility = Visibility.Visible;
        }

        public void ClearLog() {
            Items.Clear();
        }
    }
}
