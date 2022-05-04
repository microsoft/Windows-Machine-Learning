using System;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using System.Threading.Tasks;
using System.Diagnostics;
using System.Runtime.CompilerServices;
using System.IO;
using WinMLSamplesGalleryNative;
using System.Runtime.InteropServices;
using System.Threading;
using Windows.UI.Core;
using muxc = Microsoft.UI.Xaml.Controls;

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

        public void CloseInferenceWindow()
        {
            // If not already completed, shut down the window
            if (windTask != null && !windTask.IsCompleted)
            {
                //windTask.Dispose();
                WinMLSamplesGalleryNative.StreamEffect.ShutDownWindow();
            }
        }

        async private void ToggleInference(object sender, RoutedEventArgs e)
        {
            isPreviewing = !isPreviewing; // Toggle the previewing bool
            if (isPreviewing)
            {
                ToggleInferenceBtn.Visibility = Visibility.Visible;
                ToggleInferenceBtnText.Text = "Close Streaming Demo";
                ToggleInferenceBtnIcon.Symbol = Symbol.Stop;
                //MainWindow.
                MainWindow.navigationView.IsEnabled = false;
                MainWindow.mainFrame.Visibility = Visibility.Collapsed;

                // Change the button text/symbol on the button to prompt user to close window on next click
                // Waiting on the task makes sure can't navigate away from this sample until the window is closed. 
                var tok = new CancellationTokenSource(); 
                windTask = new Task(
                    () => WinMLSamplesGalleryNative.StreamEffect.LaunchNewWindow(modelPath), tok.Token);
                // Wait doesn't actually throw away input to the ui
                //windTask.Wait();
                //Task close = windTask.ContinueWith((antecendent) => { 
                //    isPreviewing = false;
                //});
                windTask.Start();
                //close.Wait();
            }

            else if (!isPreviewing)
            {
                ToggleInferenceBtnText.Text = "Launch Streaming Demo";
                ToggleInferenceBtnIcon.Symbol = Symbol.NewWindow;
                ToggleInferenceBtn.Visibility = Visibility.Visible;

                CloseInferenceWindow();
                // TODO: Implement the ShutDownWindow function
                //ShutDownWindow();
            }

        }

    }
}
