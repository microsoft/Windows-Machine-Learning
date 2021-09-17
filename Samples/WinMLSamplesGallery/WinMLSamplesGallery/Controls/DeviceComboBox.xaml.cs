using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;

namespace WinMLSamplesGallery.Controls
{
    public sealed partial class DeviceComboBox : UserControl
    {
        public int SelectedIndex = 0;
        public DeviceComboBox()
        {
            this.InitializeComponent();
        }

        private void changeSelectedIndex(object sender, RoutedEventArgs e)
        {
            SelectedIndex = DeviceBox.SelectedIndex;
        }
    }
}
