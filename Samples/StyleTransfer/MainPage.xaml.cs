using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Data;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Navigation;
using Windows.AI.MachineLearning;
using Windows.Media.Playback;
using Windows.Media.Core;
using Windows.Media.Capture;
using Windows.Media.MediaProperties;
using Windows.Storage;
using Windows.Media.Capture.Frames;
using System.Diagnostics;
using Windows.System.Display;
using Windows.Graphics.Display;
using System.Threading.Tasks;
using Windows.Media.Effects;

// The Blank Page item template is documented at https://go.microsoft.com/fwlink/?LinkId=402352&clcid=0x409

namespace StyleTransfer
{


    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class MainPage : Page
    {

#pragma warning disable CS0169 // The field 'MainPage._viewModel' is never used
        private AppViewModel _viewModel;
#pragma warning restore CS0169 // The field 'MainPage._viewModel' is never used

        public MainPage()
        {
            this.InitializeComponent();
            this.DataContext = new AppViewModel();
        }
    }

}
