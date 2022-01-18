using Microsoft.AI.MachineLearning;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using System;
using System.Collections.Generic;
using System.IO;
using System.Threading.Tasks;
using Windows.Graphics.Imaging;
using Windows.Media;
using Windows.Storage;
using WinMLSamplesGalleryNative;

namespace WinMLSamplesGallery.Samples
{
    public sealed partial class AdapterSelection : Page
    {
        List<string> adapter_options;
        LearningModelDevice device;
        public AdapterSelection()
        {
            this.InitializeComponent();
            adapter_options = new List<string> {
                "CPU",
                "DirectX",
                "DirectXHighPerformance",
                "DirectXMinPower"
            };
            device = new LearningModelDevice(LearningModelDeviceKind.Cpu);
            selectedDeviceKind.Text = "Cpu";

            var adapters_arr = WinMLSamplesGalleryNative.AdapterList.GetAdapters();
            var adapters = new List<string>(adapters_arr);

            adapter_options.AddRange(adapters);
            AdapterListView.ItemsSource = adapter_options;
        }

        private void ChangeAdapter(object sender, RoutedEventArgs e)
        {
            var device_kind_str = adapter_options[AdapterListView.SelectedIndex];
            if (AdapterListView.SelectedIndex < 4)
            {
                device = new LearningModelDevice(
                    GetLearningModelDeviceKind(device_kind_str));
                toggleCodeSnippet(true);
            }
            else
            {
                device = WinMLSamplesGalleryNative.AdapterList.CreateLearningModelDeviceFromAdapter(device_kind_str);
                toggleCodeSnippet(false);
            }
        }

        private LearningModelDeviceKind GetLearningModelDeviceKind(string device_kind_str)
        {
            if (device_kind_str == "CPU")
            {
                selectedDeviceKind.Text = "Cpu";
                return LearningModelDeviceKind.Cpu;
            }
            else if (device_kind_str == "DirectX")
            {
                selectedDeviceKind.Text = "DirectX";
                return LearningModelDeviceKind.DirectX;
            }
            else if (device_kind_str == "DirectXHighPerformance")
            {
                selectedDeviceKind.Text = "DirectXHighPerformance";
                return LearningModelDeviceKind.DirectXHighPerformance;
            }
            else
            {
                selectedDeviceKind.Text = "DirectXMinPower";
                return LearningModelDeviceKind.DirectXMinPower;
            }
        }

        private void toggleCodeSnippet(bool show)
        {
            if (show)
            {
                CodeSnippet.Visibility = Visibility.Visible;
                CodeSnippetComboBox.Visibility = Visibility.Visible;
                ViewSourCodeText.Visibility = Visibility.Collapsed;
            } else
            {
                CodeSnippet.Visibility = Visibility.Collapsed;
                CodeSnippetComboBox.Visibility = Visibility.Collapsed;
                ViewSourCodeText.Visibility = Visibility.Visible;
            }
        }
    }
}
