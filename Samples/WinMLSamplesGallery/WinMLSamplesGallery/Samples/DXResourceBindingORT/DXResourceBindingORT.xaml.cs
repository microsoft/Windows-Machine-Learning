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
using System.Threading.Tasks;

namespace WinMLSamplesGallery.Samples
{
    public class DXClassifierResult
    {
        public int index { get; private set; }
        public string label { get; private set; }
        public float probability { get; private set; }

        public DXClassifierResult(int i, string lab, float prob)
        {
            index = i;
            label = lab;
            probability = prob;
        }
    }

    public sealed partial class DXResourceBindingORT : Page
    {
        public DXResourceBindingORT()
        {
            this.InitializeComponent();
            //WinMLSamplesGalleryNative.DXResourceBinding.BindDXResourcesUsingORT();
        }

        private async void LaunchWindow(object sender, RoutedEventArgs e)
        {
            //Task.Run(() => WinMLSamplesGalleryNative.DXResourceBinding.LaunchWindow());
            WinMLSamplesGalleryNative.DXResourceBinding.LaunchWindow();
            //WinMLSamplesGalleryNative.DXResourceBinding.EvalORT();
            //System.Threading.Thread.Sleep(2000);

            //for (int i = 0; i < 10; i++)
            //{
            //    float[] results = await Task.Run(() => WinMLSamplesGalleryNative.DXResourceBinding.EvalORT());
            //    UpdateClassification(results);
            //    System.Diagnostics.Debug.WriteLine("Updated ui with eval");
            //    //System.Threading.Thread.Sleep(10000);
            //}



            //System.Diagnostics.Debug.WriteLine("In C# code");
            //for(int i = 0; i < results_lst.Count; i++)
            //{
            //    System.Diagnostics.Debug.WriteLine("indicies[i]: " + indices[i].ToString());
            //    //System.Diagnostics.Debug.WriteLine(i.ToString() + ": " + results_lst[i].ToString());
            //}
        }

        //private async Task<float[]> ClassifyFrame()
        //{
        //    return WinMLSamplesGalleryNative.DXResourceBinding.EvalORT();
        //}

        void UpdateClassification(float[] results)
        {
            var results_lst = new List<float>(results);
            List<int> indices = Enumerable.Range(0, 1000).ToList();
            indices.Sort((x, y) => results_lst[y].CompareTo(results_lst[x]));
            List<int> top_10_indices = indices.Take(10).ToList();

            List<DXClassifierResult> final_results = new List<DXClassifierResult>();
            for (int i = 0; i < 10; i++)
            {
                final_results.Add(new DXClassifierResult(i, ClassificationLabels.ImageNet[top_10_indices[i]],
                    top_10_indices[i]));
            }

            BaseExample.ItemsSource = final_results;
        }
    }
}
