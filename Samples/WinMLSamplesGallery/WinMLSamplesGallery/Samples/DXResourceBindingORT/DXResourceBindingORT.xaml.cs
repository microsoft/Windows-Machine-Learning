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
using System.Linq;

namespace WinMLSamplesGallery.Samples
{
    public class DXClassifierResult
    {
        public int index { get; private set; }
        public string label { get; private set; }
        public DXClassifierResult(int i, string lab)
        {
            index = i;
            label = lab;
        }
    }

    public sealed partial class DXResourceBindingORT : Page
    {
        private bool pageExited = false;
        public DXResourceBindingORT()
        {
            this.InitializeComponent();
        }

        private async void LaunchWindow(object sender, RoutedEventArgs e)
        {
            ToggleLaunchWindowButton();
            ToggleLoadingTxtVisibility();

            await Task.Run(() => WinMLSamplesGalleryNative.DXResourceBinding.LaunchWindow());

            ToggleLoadingTxtVisibility();
            ClassifyCube();
        }

        private async void ClassifyCube()
        {
            int i = 0;
            while (true)
            {
                if (pageExited)
                {
                    WinMLSamplesGalleryNative.DXResourceBinding.CloseWindow();
                    break;
                }
                float[] results = await Task.Run(() => WinMLSamplesGalleryNative.DXResourceBinding.EvalORT());
                UpdateClassificationResults(results);
                System.Threading.Thread.Sleep(1000);
                i++;
            }
            ToggleLaunchWindowButton();
            pageExited = false;
        }

        private void UpdateClassificationResults(float[] results)
        {
            var results_lst = new List<float>(results);
            List<int> top_1k_imagenet_indices = Enumerable.Range(0, 1000).ToList();
            top_1k_imagenet_indices.Sort((x, y) => results_lst[y].CompareTo(results_lst[x]));
            // The top 10 results represented by their indices in the list of ImageNet labels
            List<int> top_10_imagenet_indices = top_1k_imagenet_indices.Take(10).ToList();

            List<DXClassifierResult> dx_classifier_results = new List<DXClassifierResult>();
            for (int i = 0; i < 10; i++)
            {
                dx_classifier_results.Add(new DXClassifierResult(i+1,
                    ClassificationLabels.ImageNet[top_10_imagenet_indices[i]]));
            }

            Top10Results.ItemsSource = dx_classifier_results;
        }

        private void ToggleLaunchWindowButton()
        {
            LaunchWindowBtn.IsEnabled = !LaunchWindowBtn.IsEnabled;
        }

        private void ToggleLoadingTxtVisibility()
        {
            if (LoadingTxt.Visibility == Visibility.Visible)
                LoadingTxt.Visibility = Visibility.Collapsed;
            else
                LoadingTxt.Visibility = Visibility.Visible;
        }

        public void StopAllEvents()
        {
            pageExited = true;
        }
    }
}
