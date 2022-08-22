using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Navigation;
using Microsoft.AI.MachineLearning;
using Microsoft.AI.MachineLearning.Experimental;
using System;

namespace WinMLSamplesGallery
{
    public sealed partial class SampleBasePage : Page
    {
        public SampleMetadata sampleMetadata;

        public SampleBasePage()
        {
            this.InitializeComponent();
        }

        protected override async void OnNavigatedTo(NavigationEventArgs e)
        {
            sampleMetadata = (SampleMetadata)e.Parameter;
            switch (sampleMetadata.Tag)
            {
                case "ImageClassifier":
                    SampleFrame.Navigate(typeof(Samples.ImageClassifier));
                    break;
                case "Batching":
                    SampleFrame.Navigate(typeof(Samples.Batching));
                    break;
                case "ImageEffects":
                    SampleFrame.Navigate(typeof(Samples.ImageEffects));
                    break;
                case "OpenCVInterop":
                    SampleFrame.Navigate(typeof(Samples.OpenCVInterop));
                    break;
                case "ImageSharpInterop":
                    SampleFrame.Navigate(typeof(Samples.ImageSharpInterop));
                    break;
                case "EncryptedModel":
                    SampleFrame.Navigate(typeof(Samples.EncryptedModel));
                    break;
                case "AdapterSelection":
                    SampleFrame.Navigate(typeof(Samples.AdapterSelection));
                    break;
                case "StreamEffect":
                    SampleFrame.Navigate(typeof(Samples.StreamEffect));
                case "DXResourceBindingORT":
                    SampleFrame.Navigate(typeof(Samples.DXResourceBindingORT));
                    break;
            }
            if (sampleMetadata.Docs.Count > 0)
                DocsHeader.Visibility = Visibility.Visible;
            else
                DocsHeader.Visibility = Visibility.Collapsed;
        }

        protected override async void OnNavigatingFrom(NavigatingCancelEventArgs e)
        {
            // Batching Sample contains background thread events that may need to be stopped.
            if (SampleFrame.SourcePageType == typeof(Samples.Batching))
            {
                var page = (Samples.Batching)SampleFrame.Content;
                page.StopAllEvents();
            }
            // Batching Sample contains background thread events that may need to be stopped.
            else if (SampleFrame.SourcePageType == typeof(Samples.StreamEffect))
            {
                var page = (Samples.StreamEffect)SampleFrame.Content;
                page.CloseInferenceWindow();
            }
            else if(SampleFrame.SourcePageType == typeof(Samples.DXResourceBindingORT))
            {
                var page = (Samples.DXResourceBindingORT)SampleFrame.Content;
                page.StopAllEvents();

            }
        }

        public static void SetModelNameForTelemetry(String modelName, String sampleName, LearningModel model)
        {
            var telemetryModelName = modelName + "_" + sampleName;
            var experimentalModel = new LearningModelExperimental(model);
            experimentalModel.SetName(telemetryModelName);
        }
    }
}
