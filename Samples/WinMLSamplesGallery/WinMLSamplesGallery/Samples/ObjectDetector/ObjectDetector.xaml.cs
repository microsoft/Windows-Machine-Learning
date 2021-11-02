using Microsoft.AI.MachineLearning;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Media.Imaging;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Runtime.InteropServices;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Windows.Foundation.Metadata;
using Windows.Graphics.Imaging;
using Windows.Media;
using Windows.Storage;
using Windows.Storage.Pickers;
using Windows.UI;
using WinMLSamplesGallery.Common;

namespace WinMLSamplesGallery.Samples
{
    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class ObjectDetector : Page
    {
        [DllImport("Kernel32.dll", CallingConvention = CallingConvention.Winapi)]
        private static extern void GetSystemTimePreciseAsFileTime(out long filetime);

        [DllImport("user32.dll", ExactSpelling = true, CharSet = CharSet.Auto, PreserveSig = true, SetLastError = false)]
        public static extern IntPtr GetActiveWindow();

        private LearningModelSession _inferenceSession = null;
        private LearningModelSession _preProcessingSession = null;

        private Dictionary<OnnxModel, string> _modelDictionary;

        LearningModelDevice dmlDevice;
        LearningModelDevice cpuDevice;

        bool initialized_ = false;

        private BitmapDecoder CurrentImageDecoder { get; set; }

        private OnnxModel CurrentModel { get; set; }

        private OnnxModel SelectedModel
        {
            get
            {
                if (AllModelsGrid == null || AllModelsGrid.SelectedItem == null)
                {
                    return OnnxModel.Unknown;
                }
                var viewModel = (OnnxModelViewModel)AllModelsGrid.SelectedItem;
                return viewModel.Tag;
            }
        }

        private readonly string[] _labels =
            {
                "person",
                "bicycle",
                "car",
                "motorbike",
                "aeroplane",
                "bus",
                "train",
                "truck",
                "boat",
                "traffic light",
                "fire hydrant",
                "stop sign",
                "parking meter",
                "bench",
                "bird",
                "cat",
                "dog",
                "horse",
                "sheep",
                "cow",
                "elephant",
                "bear",
                "zebra",
                "giraffe",
                "backpack",
                "umbrella",
                "handbag",
                "tie",
                "suitcase",
                "frisbee",
                "skis",
                "snowboard",
                "sports ball",
                "kite",
                "baseball bat",
                "baseball glove",
                "skateboard",
                "surfboard",
                "tennis racket",
                "bottle",
                "wine glass",
                "cup",
                "fork",
                "knife",
                "spoon",
                "bowl",
                "banana",
                "apple",
                "sandwich",
                "orange",
                "broccoli",
                "carrot",
                "hot dog",
                "pizza",
                "donut",
                "cake",
                "chair",
                "sofa",
                "pottedplant",
                "bed",
                "diningtable",
                "toilet",
                "tvmonitor",
                "laptop",
                "mouse",
                "remote",
                "keyboard",
                "cell phone",
                "microwave",
                "oven",
                "toaster",
                "sink",
                "refrigerator",
                "book",
                "clock",
                "vase",
                "scissors",
                "teddy bear",
                "hair drier",
                "toothbrush"
        };

        internal struct DetectionResult
        {
            public string label;
            public List<float> bbox;
            public double prob;
        }

        class Comparer : IComparer<DetectionResult>
        {
            public int Compare(DetectionResult x, DetectionResult y)
            {
                return y.prob.CompareTo(x.prob);
            }
        }

        private bool IsCpu
        {
            get
            {
                return DeviceComboBox.SelectedIndex == 0;
            }
        }

        public ObjectDetector()
        {
            this.InitializeComponent();

            dmlDevice = new LearningModelDevice(LearningModelDeviceKind.DirectX);
            cpuDevice = new LearningModelDevice(LearningModelDeviceKind.Cpu);

            CurrentModel = OnnxModel.YoloV3;
            var allModels = new List<OnnxModelViewModel> {
                new OnnxModelViewModel { Tag = OnnxModel.YoloV3, Title = "YoloV3" },
                new OnnxModelViewModel { Tag = OnnxModel.YoloV4Repo, Title = "YoloV4 Repo" },
                new OnnxModelViewModel { Tag = OnnxModel.YoloV4, Title = "YoloV4" },
#if USE_LARGE_MODELS
#endif
                };

            AllModelsGrid.ItemsSource = allModels;
            AllModelsGrid.SelectRange(new ItemIndexRange(0, 1));
            AllModelsGrid.SelectionChanged += AllModelsGrid_SelectionChanged;

            EnsureInitialized();

            initialized_ = true;
        }

        private void EnsureInitialized()
        {
            if (_modelDictionary == null)
            {
                _modelDictionary = new Dictionary<OnnxModel, string>{
                    { OnnxModel.YoloV3,     "ms-appx:///Models/yolov3.onnx" },
                    { OnnxModel.YoloV4Repo, "ms-appx:///Models/yolov4.repo.onnx" },
                    { OnnxModel.YoloV4,     "ms-appx:///Models/yolov4.onnx" },
#if USE_LARGE_MODELS
                    // Large Models
#endif
                };
            }
            InitializeWindowsMachineLearning();
        }

        private void InitializeWindowsMachineLearning()
        {
            if (CurrentImageDecoder != null)
            {
                LearningModel preProcessingModel = null;
                switch (SelectedModel)
                {
                    case OnnxModel.YoloV3:
                        preProcessingModel = TensorizationModels.YoloV3(4, (long)CurrentImageDecoder.PixelHeight, (long)CurrentImageDecoder.PixelWidth);
                        break;
                    case OnnxModel.YoloV4Repo:
                        preProcessingModel = TensorizationModels.YoloV4Repo(1, 4, (long)CurrentImageDecoder.PixelHeight, (long)CurrentImageDecoder.PixelWidth, 416, 416);
                        break;
                    case OnnxModel.YoloV4:
                        preProcessingModel = TensorizationModels.YoloV4(1, 4, (long)CurrentImageDecoder.PixelHeight, (long)CurrentImageDecoder.PixelWidth, 416, 416);
                        break;
#if USE_LARGE_MODELS
                 // Large Models
#endif
                };

                _preProcessingSession = CreateLearningModelSession(preProcessingModel);
            }

            var model = SelectedModel;
            if (model != CurrentModel)
            {
                var modelPath = _modelDictionary[model];
                _inferenceSession = CreateLearningModelSession(modelPath);
                CurrentModel = model;
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
                CloseModelOnSessionCreation = true              // Close the model to prevent extra memory usage
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

        private void DeviceComboBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
        }

        private void OpenButton_Clicked(object sender, RoutedEventArgs e)
        {
            var CurrentImageDecoder = ImageHelper.PickImageFileAsBitmapDecoder();
            if (CurrentImageDecoder != null)
            {
                BasicGridView.SelectedItem = null;
                TryPerformInference();
            }
        }

        private void SampleInputsGridView_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            var gridView = sender as GridView;
            var thumbnail = gridView.SelectedItem as WinMLSamplesGallery.Controls.Thumbnail;
            if (thumbnail != null)
            {
                CurrentImageDecoder = ImageHelper.CreateBitmapDecoderFromPath(thumbnail.ImageUri);
                TryPerformInference();
            }
        }

        private void Detect(BitmapDecoder decoder)
        {
            Results.ClearLog();

            // Tensorize
            LearningModelSession preProcessingSession = null;
            object inferenceInput = null;
            object input = null;
            if (ApiInformation.IsTypePresent("Windows.Media.VideoFrame") && false)
            {
                var softwareBitmap = decoder.GetSoftwareBitmapAsync().GetAwaiter().GetResult();
                inferenceInput = VideoFrame.CreateWithSoftwareBitmap(softwareBitmap);
            }
            else
            {
                var pixelDataProvider = decoder.GetPixelDataAsync(BitmapPixelFormat.Rgba8, BitmapAlphaMode.Premultiplied, new BitmapTransform(), ExifOrientationMode.RespectExifOrientation, ColorManagementMode.ColorManageToSRgb).GetAwaiter().GetResult();
                var bytes = pixelDataProvider.DetachPixelData();
                var buffer = bytes.AsBuffer(); // Does this do a copy??
                input = TensorUInt8Bit.CreateFromBuffer(new long[] { 1, buffer.Length }, buffer);
                preProcessingSession = _preProcessingSession;
            }

            // 3 channel NCHW
            if (preProcessingSession != null)
            {
                inferenceInput = TensorFloat.Create(new long[] { 1, 416, 416, 3 });
                var b = new LearningModelBinding(preProcessingSession);
                b.Bind(preProcessingSession.Model.InputFeatures[0].Name, input);
                b.Bind(preProcessingSession.Model.OutputFeatures[0].Name, inferenceInput);
                preProcessingSession.Evaluate(b, "");
            }

            // Infrence
            var inferenceBinding = new LearningModelBinding(_inferenceSession);
            inferenceBinding.Bind(_inferenceSession.Model.InputFeatures[0].Name, inferenceInput);
            var results = _inferenceSession.Evaluate(inferenceBinding, "");

            var output = results.Output(0) as TensorFloat;
            var data = output.GetAsVectorView();
            var detections = ParseResult(data.ToList<float>().ToArray());

            var cp = new Comparer();
            detections.Sort(cp);
            var final = NMS(detections);

            int num = 0;
            foreach (var result in final)
            {
                num++;
                Results.Log(result.label, 0);
                if (num > 10) { break; }

            }
        }

        private static SoftwareBitmap CreateSoftwareBitmapFromStorageFile(StorageFile file)
        {
            var stream = file.OpenAsync(FileAccessMode.Read).GetAwaiter().GetResult();
            var decoder = BitmapDecoder.CreateAsync(stream).GetAwaiter().GetResult();
            return decoder.GetSoftwareBitmapAsync().GetAwaiter().GetResult();
        }

        private void RenderImageInMainPanel(SoftwareBitmap softwareBitmap)
        {
            SoftwareBitmap displayBitmap = softwareBitmap;
            //Image control only accepts BGRA8 encoding and Premultiplied/no alpha channel. This checks and converts
            //the SoftwareBitmap we want to bind.
            if (displayBitmap.BitmapPixelFormat != BitmapPixelFormat.Bgra8 ||
                displayBitmap.BitmapAlphaMode != BitmapAlphaMode.Premultiplied)
            {
                displayBitmap = SoftwareBitmap.Convert(displayBitmap, BitmapPixelFormat.Bgra8, BitmapAlphaMode.Premultiplied);
            }

            // get software bitmap souce
            var source = new SoftwareBitmapSource();
            source.SetBitmapAsync(displayBitmap).GetAwaiter();
            // draw the input image
            InputImage.Source = source;
        }


        // parse the result from WinML evaluation results to self defined object struct
        private List<DetectionResult> ParseResult(float[] results)
        {
            int c_values = 84;
            int c_boxes = results.Length / c_values;
            float confidence_threshold = 0.85f;
            List<DetectionResult> detections = new List<DetectionResult>();
            for (int i_box = 0; i_box < c_boxes; i_box++)
            {
                float max_prob = 0.0f;
                int label_index = -1;
                for (int j_confidence = 4; j_confidence < c_values; j_confidence++)
                {
                    int index = i_box * c_values + j_confidence;
                    if (results[index] > max_prob)
                    {
                        max_prob = results[index];
                        label_index = j_confidence - 4;
                    }
                }
                if (max_prob > confidence_threshold)
                {
                    List<float> bbox = new List<float>();
                    bbox.Add(results[i_box * c_values + 0]);
                    bbox.Add(results[i_box * c_values + 1]);
                    bbox.Add(results[i_box * c_values + 2]);
                    bbox.Add(results[i_box * c_values + 3]);

                    detections.Add(new DetectionResult()
                    {
                        label = _labels[label_index],
                        bbox = bbox,
                        prob = max_prob
                    });
                }
            }
            return detections;
        }

        // Non-maximum Suppression(NMS), a technique which filters the proposals 
        // based on Intersection over Union(IOU)
        private List<DetectionResult> NMS(IReadOnlyList<DetectionResult> detections,
            float IOU_threshold = 0.45f,
            float score_threshold = 0.3f)
        {
            List<DetectionResult> final_detections = new List<DetectionResult>();
            for (int i = 0; i < detections.Count; i++)
            {
                int j = 0;
                for (j = 0; j < final_detections.Count; j++)
                {
                    if (ComputeIOU(final_detections[j], detections[i]) > IOU_threshold)
                    {
                        break;
                    }
                }
                if (j == final_detections.Count)
                {
                    final_detections.Add(detections[i]);
                }
            }
            return final_detections;
        }

        // Compute Intersection over Union(IOU)
        private float ComputeIOU(DetectionResult DRa, DetectionResult DRb)
        {
            float ay1 = DRa.bbox[0];
            float ax1 = DRa.bbox[1];
            float ay2 = DRa.bbox[2];
            float ax2 = DRa.bbox[3];
            float by1 = DRb.bbox[0];
            float bx1 = DRb.bbox[1];
            float by2 = DRb.bbox[2];
            float bx2 = DRb.bbox[3];

            //Debug.Assert(ay1 < ay2);
            //Debug.Assert(ax1 < ax2);
            //Debug.Assert(by1 < by2);
            //Debug.Assert(bx1 < bx2);

            // determine the coordinates of the intersection rectangle
            float x_left = Math.Max(ax1, bx1);
            float y_top = Math.Max(ay1, by1);
            float x_right = Math.Min(ax2, bx2);
            float y_bottom = Math.Min(ay2, by2);

            if (x_right < x_left || y_bottom < y_top)
                return 0;
            float intersection_area = (x_right - x_left) * (y_bottom - y_top);
            float bb1_area = (ax2 - ax1) * (ay2 - ay1);
            float bb2_area = (bx2 - bx1) * (by2 - by1);
            float iou = intersection_area / (bb1_area + bb2_area - intersection_area);

            //Debug.Assert(iou >= 0 && iou <= 1);
            return iou;
        }

        private void AllModelsGrid_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            TryPerformInference();
        }

        private void TryPerformInference()
        {
            if (CurrentImageDecoder != null)
            {
                EnsureInitialized();

                // Draw the image to classify in the Image control
                var softwareBitmap = CurrentImageDecoder.GetSoftwareBitmapAsync().GetAwaiter().GetResult();
                RenderingHelpers.BindSoftwareBitmapToImageControl(InputImage, softwareBitmap);
                Detect(CurrentImageDecoder);
            }
        }
    }
}