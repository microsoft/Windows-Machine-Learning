using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.AI.MachineLearning;
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

namespace TesterApp
{
    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class MainPage : Page
    {
        public MainPage()
        {
            this.InitializeComponent();
        }

        private void ButtonRun_Click(object sender, RoutedEventArgs e)
        {
            var helper = new DXCore_WinRTComponent.DXCoreHelper();

            // Graphics-capable adapter

            LearningModelDevice gfxLMDevice = helper.GetDeviceFromGraphicsAdapter();

            TextOutput.Text += "Creating a WinML LearningModelDevice from the first valid graphics-capable adapter (e.g. a GPU): ";

            if (gfxLMDevice != null)
            {
                TextOutput.Text += "Successful (Adapter ID " + gfxLMDevice.AdapterId.HighPart + "-" + gfxLMDevice.AdapterId.LowPart + ")\n";
            }
            else
            {
                TextOutput.Text += "Unsuccessful (no valid adapter found)\n";
            }

            // Compute-only adapter

            LearningModelDevice compLMDevice = helper.GetDeviceFromComputeOnlyAdapter();

            TextOutput.Text += "Creating a WinML LearningModelDevice from the first valid compute-only adapter (e.g. a VPU): ";

            if (compLMDevice != null)
            {
                TextOutput.Text += "Successful (Adapter ID " + compLMDevice.AdapterId.HighPart + "-" + compLMDevice.AdapterId.LowPart + ")\n";
            }
            else
            {
                TextOutput.Text += "Unsuccessful (no valid adapter found)\n";
            }

            // Intel® Movidius™ VPU

            LearningModelDevice vpuLMDevice = helper.GetDeviceFromVpuAdapter();

            TextOutput.Text += "Creating a WinML LearningModelDevice hardcoded to select the Intel® Movidius™ VPU: ";

            if (vpuLMDevice != null)
            {
                TextOutput.Text += "Successful (Adapter ID " + vpuLMDevice.AdapterId.HighPart + "-" + vpuLMDevice.AdapterId.LowPart + ")\n";
            }
            else
            {
                TextOutput.Text += "Unsuccessful (no valid adapter found)\n";
            }

            // All hardware adapters

            IReadOnlyList<LearningModelDevice> hwLMDevices = helper.GetAllHardwareDevices();

            TextOutput.Text += "Creating WinML LearningModelDevices for every hardware adapter: ";

            if (hwLMDevices != null)
            {
                TextOutput.Text += "Successful (" + hwLMDevices.Count + " adapters total)\n";
                foreach (LearningModelDevice dev in hwLMDevices)
                {
                    TextOutput.Text += "    Adapter: " + dev.AdapterId.HighPart + "-" + dev.AdapterId.LowPart + "\n";
                }
            }
            else
            {
                TextOutput.Text += "Unsuccessful (no valid adapters found)\n";
            }
        }
    }
}
