using System;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using System.Runtime.InteropServices;
using Windows.Storage.Pickers;
using Windows.Storage;
using Windows.Graphics.Imaging;
using Windows.Media;
using Microsoft.AI.MachineLearning;
using Microsoft.UI.Xaml.Media.Imaging;

namespace WinMLSamplesGallery.Samples
{

    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class BatchedInference : Page
    {
        [DllImport("user32.dll", ExactSpelling = true, CharSet = CharSet.Auto, PreserveSig = true, SetLastError = false)]
        public static extern IntPtr GetActiveWindow();

        private static Dictionary<long, string> labels_;
        private LearningModelSession inferenceSession_;
        private LearningModelSession preProcessingSession_;
        private LearningModelSession postProcessingSession_;

        private SoftwareBitmap selected_image_ = null;
        Classifier currentModel_ = Classifier.SqueezeNet;
        Classifier loadedModel_ = Classifier.NotSet;
        const long BatchSize = 1;
        const long TopK = 10;

        private Dictionary<Classifier, string> modelDictionary_;
        private Dictionary<Classifier, Func<LearningModel>> postProcessorDictionary_;
        private Dictionary<Classifier, Func<LearningModel>> preProcessorDictionary_;

        private static Dictionary<long, string> imagenetLabels_;
        private static Dictionary<long, string> ilsvrc2013Labels_;

        public BatchedInference()
        {
            this.InitializeComponent();
        }

        private void OpenButton_Clicked(object sender, RoutedEventArgs e)
        {
            var file = PickFile();
            if (file != null)
            {

            }
        }

        private StorageFile PickFile()
        {
            FileOpenPicker openPicker = new FileOpenPicker();
            openPicker.ViewMode = PickerViewMode.Thumbnail;
            openPicker.FileTypeFilter.Add(".jpg");
            openPicker.FileTypeFilter.Add(".bmp");
            openPicker.FileTypeFilter.Add(".png");
            openPicker.FileTypeFilter.Add(".jpeg");

            // When running on win32, FileOpenPicker needs to know the top-level hwnd via IInitializeWithWindow::Initialize.
            if (Window.Current == null)
            {
                var picker_unknown = Marshal.GetComInterfaceForObject(openPicker, typeof(IInitializeWithWindow));
                var initializeWithWindowWrapper = (IInitializeWithWindow)Marshal.GetTypedObjectForIUnknown(picker_unknown, typeof(IInitializeWithWindow));
                IntPtr hwnd = GetActiveWindow();
                initializeWithWindowWrapper.Initialize(hwnd);
            }

            return openPicker.PickSingleFileAsync().GetAwaiter().GetResult();
        }

        private static SoftwareBitmap CreateSoftwareBitmapFromStorageFile(StorageFile file)
        {
            var stream = file.OpenAsync(FileAccessMode.Read).GetAwaiter().GetResult();
            var decoder = BitmapDecoder.CreateAsync(stream).GetAwaiter().GetResult();
            return decoder.GetSoftwareBitmapAsync().GetAwaiter().GetResult();
        }

        private void SelectImage(object sender, SelectionChangedEventArgs e)
        {
            Console.WriteLine("CHANGED CHANGED CHANGED");
            var gridView = sender as GridView;
            var thumbnail = gridView.SelectedItem as WinMLSamplesGallery.Controls.Thumbnail;
            if (thumbnail != null)
            {
                var image = thumbnail.ImageUri;
                var file = StorageFile.GetFileFromApplicationUriAsync(new Uri(image)).GetAwaiter().GetResult();
                selected_image_ = CreateSoftwareBitmapFromStorageFile(file);
                StartInference(sender, e);
            }
        }

        private void EnsureInit()
        {
            if (imagenetLabels_ == null)
            {
                imagenetLabels_ = LoadLabels("ms-appx:///InputData/sysnet.txt");
            }

            if (ilsvrc2013Labels_ == null)
            {
                ilsvrc2013Labels_ = LoadLabels("ms-appx:///InputData/ilsvrc2013.txt");
            }

            if (modelDictionary_ == null)
            {
                modelDictionary_ = new Dictionary<Classifier, string>
                {
                    { Classifier.SqueezeNet, "ms-appx:///Models/squeezenet1.1-7.onnx" },

                };
            }

            if (postProcessorDictionary_ == null)
            {
                postProcessorDictionary_ = new Dictionary<Classifier, Func<LearningModel>>{
                    { Classifier.SqueezeNet, () => TensorizationModels.SoftMaxThenTopK(TopK) },
                };
            }

            InitializeWindowsMachineLearning(currentModel_);
        }

        private void InitializeWindowsMachineLearning(Classifier model)
        {
            if (currentModel_ != loadedModel_)
            {
                var modelPath = modelDictionary_[model];
                inferenceSession_ = CreateLearningModelSession(modelPath);
                preProcessingSession_ = null;
                Func<LearningModel> postProcessor = () => TensorizationModels.SoftMaxThenTopK(TopK);
                postProcessingSession_ = CreateLearningModelSession(postProcessor());

                if (currentModel_ == Classifier.RCNN_ILSVRC13)
                {
                    labels_ = ilsvrc2013Labels_;
                }
                else
                {
                    labels_ = imagenetLabels_;
                }

                loadedModel_ = currentModel_;
            }
        }

        private LearningModelSession CreateLearningModelSession(string modelPath)
        {
            var model = CreateLearningModel(modelPath);
            var session = CreateLearningModelSession(model);
            return session;
        }

        private LearningModelSession CreateLearningModelSession(LearningModel model)
        {
            var kind =
                (DeviceComboBox.SelectedIndex == 0) ?
                    LearningModelDeviceKind.Cpu :
                    LearningModelDeviceKind.DirectXHighPerformance;
            var device = new LearningModelDevice(kind);
            var options = new LearningModelSessionOptions()
            {
                CloseModelOnSessionCreation = true // Close the model to prevent extra memory usage
            };
            var session = new LearningModelSession(model, device, options);
            return session;
        }

        private static LearningModel CreateLearningModel(string modelPath)
        {
            var uri = new Uri(modelPath);
            var file = StorageFile.GetFileFromApplicationUriAsync(uri).GetAwaiter().GetResult();
            return LearningModel.LoadFromStorageFileAsync(file).GetAwaiter().GetResult();
        }

        private void StartInference(object sender, RoutedEventArgs e)
        {
            var file = StorageFile.GetFileFromApplicationUriAsync(new Uri("ms-appx:///InputData/hummingbird.jpg")).GetAwaiter().GetResult();
            selected_image_ = CreateSoftwareBitmapFromStorageFile(file);
            if (selected_image_ != null)
            {
                EnsureInit();
                var (labels, probabilities) = Classify(selected_image_);
            }
        }

        private static Dictionary<long, string> LoadLabels(string csvFile)
        {
            var file = StorageFile.GetFileFromApplicationUriAsync(new Uri(csvFile)).GetAwaiter().GetResult();
            var text = Windows.Storage.FileIO.ReadTextAsync(file).GetAwaiter().GetResult();
            var labels = new Dictionary<long, string>();
            var records = text.Split(Environment.NewLine);
            foreach (var record in records)
            {
                var fields = record.Split(",", 2);
                if (fields.Length == 2)
                {
                    var index = long.Parse(fields[0]);
                    labels[index] = fields[1];
                }
            }
            return labels;
        }

        private (IEnumerable<string>, IReadOnlyList<float>) Classify(SoftwareBitmap softwareBitmap)
        {
            PerformanceMetricsMonitor.ClearLog();

            long start, stop;

            var input = (object)VideoFrame.CreateWithSoftwareBitmap(softwareBitmap);

            // PreProcess
            start = HighResolutionClock.UtcNow();
            object preProcessedOutput = input;
            if (preProcessingSession_ != null)
            {
                var preProcessedResults = Evaluate(preProcessingSession_, input);
                preProcessedOutput = preProcessedResults.Outputs.First().Value;
                var preProcessedOutputTF = preProcessedOutput as TensorFloat;
                var shape = preProcessedOutputTF.Shape;
                System.Diagnostics.Debug.WriteLine("shape = {0}, {1}, {2}, {3}", shape[0], shape[1], shape[2], shape[3]);
            }
            stop = HighResolutionClock.UtcNow();
            var preprocessDuration = HighResolutionClock.DurationInMs(start, stop);

            // Inference
            start = HighResolutionClock.UtcNow();
            var inferenceResults = Evaluate(inferenceSession_, preProcessedOutput);
            var inferenceOutput = inferenceResults.Outputs.First().Value as TensorFloat;
            stop = HighResolutionClock.UtcNow();
            var inferenceDuration = HighResolutionClock.DurationInMs(start, stop);

            // PostProcess
            start = HighResolutionClock.UtcNow();
            var postProcessedOutputs = Evaluate(postProcessingSession_, inferenceOutput);
            var topKValues = postProcessedOutputs.Outputs["TopKValues"] as TensorFloat;
            var topKIndices = postProcessedOutputs.Outputs["TopKIndices"] as TensorInt64Bit;

            // Return results
            var probabilities = topKValues.GetAsVectorView();
            var indices = topKIndices.GetAsVectorView();
            var labels = indices.Select((index) => labels_[index]);
            stop = HighResolutionClock.UtcNow();
            var postProcessDuration = HighResolutionClock.DurationInMs(start, stop);

            PerformanceMetricsMonitor.Log("Pre-process", preprocessDuration);
            PerformanceMetricsMonitor.Log("Inference", inferenceDuration);
            PerformanceMetricsMonitor.Log("Post-process", postProcessDuration);

            return (labels, probabilities);
        }

        private static LearningModelEvaluationResult Evaluate(LearningModelSession session, object input)
        {
            // Create the binding
            var binding = new LearningModelBinding(session);

            // Create an emoty output, that will keep the output resources on the GPU
            // It will be chained into a the post processing on the GPU as well
            var output = TensorFloat.Create();

            // Bind inputs and outputs
            // For squeezenet these evaluate to "data", and "squeezenet0_flatten0_reshape0"
            string inputName = session.Model.InputFeatures[0].Name;
            string outputName = session.Model.OutputFeatures[0].Name;
            binding.Bind(inputName, input);
            binding.Bind(outputName, output);

            // Evaluate
            return session.Evaluate(binding, "");
        }

        private void DeviceComboBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
        }

    }
}
