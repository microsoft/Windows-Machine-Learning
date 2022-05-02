using System;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using System.Threading.Tasks;
using System.Diagnostics;
using System.Runtime.CompilerServices;
using System.IO;
using WinMLSamplesGalleryNative;
using System.Runtime.InteropServices;


namespace WinMLSamplesGallery.Samples
{


    public sealed partial class StreamEffect : Page
    {
        string modelPath;
        bool isPreviewing = false;
        IntPtr currentHwnd;
        Task windTask;

        [DllImport("user32.dll", CharSet = CharSet.Auto, SetLastError = true)]
        static extern IntPtr GetForegroundWindow();

        public StreamEffect()
        {
            this.InitializeComponent();
            // TODO: Currently the WinMLSamplesGalleryNative component will load
            // the wrong version of the Microsoft.AI.MachineLearning.dll.
            // To work around this, we make a dummy call to the builder to
            // ensure that the dll is loaded.
            var builder = Microsoft.AI.MachineLearning.Experimental.LearningModelBuilder.Create(11);

            currentHwnd = GetForegroundWindow();
            //var modelName = "mosaic.onnx";
            modelPath = Path.Join(Windows.ApplicationModel.Package.Current.InstalledLocation.Path, "Models");
            
        }

        async private void ToggleInference(object sender, RoutedEventArgs e)
        {
            isPreviewing = !isPreviewing; // Toggle the previewing bool
            if (isPreviewing)
            {
                ToggleInferenceBtnText.Text = "Stop Inference";
                ToggleInferenceBtnIcon.Symbol = Symbol.Stop;

                // Change the button text/symbol on the button to prompt user to close window on next click
                // Waiting on the task makes sure can't navigate away from this sample until the window is closed. 
                windTask = Task.Run(() => WinMLSamplesGalleryNative.StreamEffect.LaunchNewWindow(modelPath));
                // Wait doesn't actually throw away input to the ui
                //windTask.Wait();
                ToggleInferenceBtn.Visibility = Visibility.Visible; 

            }

            else if (!isPreviewing)
            {
                ToggleInferenceBtnText.Text = "Start Inference";
                ToggleInferenceBtnIcon.Symbol = Symbol.NewWindow;
                ToggleInferenceBtn.Visibility = Visibility.Visible;

                if (!windTask.IsCompleted)
                {
                    WinMLSamplesGalleryNative.StreamEffect.ShutDownWindow();
                }

                // TODO: Implement the ShutDownWindow function
                //ShutDownWindow();
            }

        }

    }
}
