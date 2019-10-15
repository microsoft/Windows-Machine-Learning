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
        private static string _labelsFileName = "Assets/Labels.json";
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

            Console.WriteLine("Getting color management mode...");
            ColorManagementMode colorManagementMode = GetColorManagementMode();

            Console.WriteLine("Loading the image...");
            ImageFeatureValue imageTensor = LoadImageFile(colorManagementMode);

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

        private static ImageFeatureValue LoadImageFile(ColorManagementMode colorManagementMode)
        {
            StorageFile imageFile = AsyncHelper(StorageFile.GetFileFromPathAsync(_imagePath));
            IRandomAccessStream stream = AsyncHelper(imageFile.OpenReadAsync());
            BitmapDecoder decoder = AsyncHelper(BitmapDecoder.CreateAsync(stream));
            SoftwareBitmap softwareBitmap = null;
            try
            {
                softwareBitmap = AsyncHelper(
                    decoder.GetSoftwareBitmapAsync(
                        decoder.BitmapPixelFormat,
                        decoder.BitmapAlphaMode,
                        new BitmapTransform(),
                        ExifOrientationMode.RespectExifOrientation,
                        colorManagementMode
                    )
                );
            }
            catch (Exception e)
            {
                Console.WriteLine("Failed to create SoftwareBitmap! Please make sure that input image is within the model's colorspace.");
                Console.WriteLine(" Exception caught.\n {0}", e);
                System.Environment.Exit(e.HResult);
            }
            softwareBitmap = SoftwareBitmap.Convert(softwareBitmap, BitmapPixelFormat.Bgra8, BitmapAlphaMode.Premultiplied);
            VideoFrame inputImage = VideoFrame.CreateWithSoftwareBitmap(softwareBitmap);
            return ImageFeatureValue.CreateFromVideoFrame(inputImage);
        }

        private static ColorManagementMode GetColorManagementMode()
        {
            // Get model color space gamma
            string gammaSpace = "";
            bool doesModelContainGammaSpaceMetadata = _model.Metadata.TryGetValue("Image.ColorSpaceGamma", out gammaSpace);
            if (!doesModelContainGammaSpaceMetadata)
            {
                Console.WriteLine("    Model does not have color space gamma information. Will color manage to sRGB by default...");
            }
            if (!doesModelContainGammaSpaceMetadata || gammaSpace.Equals("SRGB", StringComparison.CurrentCultureIgnoreCase))
            {
                return ColorManagementMode.ColorManageToSRgb;
            }
            // Due diligence should be done to make sure that the input image is within the model's colorspace. There are multiple non-sRGB color spaces.
            Console.WriteLine("    Model metadata indicates that color gamma space is : {0}. Will not manage color space...", gammaSpace);
            return ColorManagementMode.DoNotColorManage;
        }

        private static void PrintResults(IReadOnlyList<float> resultVector)
        {
            // load the labels
            LoadLabels();

            List<(int index, float probability)> indexedResults = new List<(int, float)>();
            for (int i = 0; i < resultVector.Count; i++)
            {
                indexedResults.Add((index: i, probability: resultVector.ElementAt(i)));
            }
            indexedResults.Sort((a, b) =>
            {
                if (a.probability < b.probability)
                {
                    return 1;
                }
                else if (a.probability > b.probability)
                {
                    return -1;
                }
                else
                {
                    return 0;
                }
            });

            for (int i = 0; i < 3; i++)
            {
                Console.WriteLine($"\"{ _labels[indexedResults[i].index]}\" with confidence of { indexedResults[i].probability}");
            }
        }
    }
}