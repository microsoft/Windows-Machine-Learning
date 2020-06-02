using System;
using System.Collections.Generic;
using System.ComponentModel;
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

// The Blank Page item template is documented at https://go.microsoft.com/fwlink/?LinkId=402352&clcid=0x409

namespace VideoStreamDemo
{
    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class MainPage : Page
    {
        public MainPage()
        {
            this.InitializeComponent();
            this.ViewModel = new PageViewModel();
        }

        // Hacky version of button click- not currently hooked up to the media buttons, though. 
        private void Button_Click(object sender, RoutedEventArgs e)
        {
            if (sender is Button b)
            {
                string name = b.Name;
                switch (name)
                {
                    case "UIButtonLiveStream":
                        ViewModel.SelectedSource = "LiveStream";
                        break;
                    case "UIButtonAcquireImage":
                        ViewModel.SelectedSource = "AcquireImage";
                        break;
                    case "UIButtonFilePick":
                        ViewModel.SelectedSource = "FilePick";
                        break;
                    case "UIButtonInking":
                        ViewModel.SelectedSource = "Inking";
                        break;
                }
            }
        }
        public PageViewModel ViewModel { get; set; }
    }
}
