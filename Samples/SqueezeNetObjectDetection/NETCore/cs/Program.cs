using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Threading;
using Windows.Storage;
using Windows.Graphics.Imaging;
using Windows.Storage.Streams;
using Windows.AI.MachineLearning;
using Windows.Foundation;
using Windows.Media;
using Newtonsoft.Json;

namespace SqueezeNetObjectDetectionNC
{
    class ImageInference
    {
        // globals
        private static LearningModelDeviceKind _deviceKind = LearningModelDeviceKind.Default;
        private static string _deviceName = "default";
        private static string _modelPath;
        private static string _imagePath;
        private static string _labelsFileName = "Labels.json";
        private static LearningModel _model = null;
        private static LearningModelSession _session;
        private static List<string> _labels = new List<string>();

        // usage: SqueezeNet [modelfile] [imagefile] [cpu|directx]
        static int Main(string[] args)
        {
            if (!ParseArgs(args))
            {
                Console.WriteLine("Usage: [executable_name] [modelfile] [imagefile] [cpu|directx]");
                return -1;
            }

            // Load and create the model 
            Console.WriteLine($"Loading modelfile '{_modelPath}' on the '{_deviceName}' device");

            int ticks = Environment.TickCount;
            _model = LearningModel.LoadFromFilePath(_modelPath);
            ticks = Environment.TickCount - ticks;
            Console.WriteLine($"model file loaded in { ticks } ticks");

            // Create the evaluation session with the model and device
            _session = new LearningModelSession(_model, new LearningModelDevice(_deviceKind));

            Console.WriteLine("Loading the image...");
            ImageFeatureValue imageTensor = LoadImageFile();

            // create a binding object from the session
            Console.WriteLine("Binding...");
            LearningModelBinding binding = new LearningModelBinding(_session);
            binding.Bind(_model.InputFeatures.ElementAt(0).Name, imageTensor);

            Console.WriteLine("Running the model...");
            ticks = Environment.TickCount;
            var results = _session.Evaluate(binding, "RunId");
            ticks = Environment.TickCount - ticks;
            Console.WriteLine($"model run took { ticks } ticks");

            // retrieve results from evaluation
            var resultTensor = results.Outputs[_model.OutputFeatures.ElementAt(0).Name] as TensorFloat;
            var resultVector = resultTensor.GetAsVectorView();
            PrintResults(resultVector);
            return 0;
        }

        static bool ParseArgs(string[] args)
        {
            if (args.Length < 2)
            {
                return false;
            }
            // get the model file
            _modelPath = args[0];
            // get the image file
            _imagePath = args[1];
            // did they pass a fourth arg?

            if (args.Length > 2)
            {
                string deviceName = args[2];
                if (deviceName == "cpu")
                {
                    _deviceKind = LearningModelDeviceKind.Cpu;
                }
                else if (deviceName == "directx")
                {
                    _deviceKind = LearningModelDeviceKind.DirectX;
                }
            }
            return true;
        }

        private static void LoadLabels()
        {
            // Parse labels from label json file.  We know the file's 
            // entries are already sorted in order.
            var fileString = File.ReadAllText(_labelsFileName);
            var fileDict = JsonConvert.DeserializeObject<Dictionary<string, string>>(fileString);
            foreach (var kvp in fileDict)
            {
                _labels.Add(kvp.Value);
            }
        }

        
        private static T AsyncHelper<T> (IAsyncOperation<T> operation) 
        {
            AutoResetEvent waitHandle = new AutoResetEvent(false);
            operation.Completed = new AsyncOperationCompletedHandler<T>((op, status) =>
            {
                waitHandle.Set();
            });
            waitHandle.WaitOne();
            return operation.GetResults();
        }

        private static ImageFeatureValue LoadImageFile()
        {
            StorageFile imageFile = AsyncHelper(StorageFile.GetFileFromPathAsync(_imagePath));
            IRandomAccessStream stream = AsyncHelper(imageFile.OpenReadAsync());
            BitmapDecoder decoder = AsyncHelper(BitmapDecoder.CreateAsync(stream));
            SoftwareBitmap softwareBitmap = AsyncHelper(decoder.GetSoftwareBitmapAsync());
            softwareBitmap = SoftwareBitmap.Convert(softwareBitmap, BitmapPixelFormat.Bgra8, BitmapAlphaMode.Premultiplied);
            VideoFrame inputImage = VideoFrame.CreateWithSoftwareBitmap(softwareBitmap);
            return ImageFeatureValue.CreateFromVideoFrame(inputImage);
        }


        private static void PrintResults(IReadOnlyList<float> resultVector)
        {
            // load the labels
            LoadLabels();
            // Find the top 3 probabilities
            List<float> topProbabilities = new List<float>() { 0.0f, 0.0f, 0.0f };
            List<int> topProbabilityLabelIndexes = new List<int>() { 0, 0, 0 };
            // SqueezeNet returns a list of 1000 options, with probabilities for each, loop through all
            for (int i = 0; i < resultVector.Count(); i++)
            {
                // is it one of the top 3?
                for (int j = 0; j < 3; j++)
                {
                    if (resultVector[i] > topProbabilities[j])
                    {
                        topProbabilityLabelIndexes[j] = i;
                        topProbabilities[j] = resultVector[i];
                        break;
                    }
                }
            }
            for (int i = 0; i < 3; i++)
            {
                Console.WriteLine($"\"{ _labels[topProbabilityLabelIndexes[i]]}\" with confidence of { topProbabilities[i]}");
            }
        }
    }
}