using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using System.Threading.Tasks;
using Windows.AI.MachineLearning;
using Windows.Devices.Enumeration;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Windows.Media;
using Windows.Media.Capture;
using Windows.Storage;
using Windows.UI;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Data;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Media.Imaging;
using Windows.UI.Xaml.Navigation;
using Windows.UI.Xaml.Shapes;

namespace YOLOv4ObjectDetection
{

    public sealed partial class MainPage : Page
    {
        private MediaCapture _media_capture;
        private Model _model;
        private DispatcherTimer _timer;
        private readonly SolidColorBrush _fill_brush = new SolidColorBrush(Colors.Transparent);
        private readonly SolidColorBrush _line_brush = new SolidColorBrush(Colors.DarkGreen);
        private readonly double _line_thickness = 2.0;

        public MainPage()
        {
            this.InitializeComponent(); 
            button_go.IsEnabled = false;
            this.Loaded += OnPageLoaded;
        }

        private void OnPageLoaded(object sender, RoutedEventArgs e)
        {
            _ = InitModelAsync();
            _ = InitCameraAsync();
        }

        private async Task InitModelAsync()
        {
            ShowStatus("Loading yolo.onnx model...");
            try
            {
                _model = new Model();
                await _model.InitModelAsync();
                ShowStatus("ready");
                button_go.IsEnabled = true;
            } 
            catch (Exception ex)
            {
                ShowStatus(ex.Message);
            }
        }

        private async Task InitCameraAsync()
        {
            if (_media_capture == null || _media_capture.CameraStreamState == Windows.Media.Devices.CameraStreamState.Shutdown || _media_capture.CameraStreamState == Windows.Media.Devices.CameraStreamState.NotStreaming)
            {
                if (_media_capture != null)
                {
                    _media_capture.Dispose();
                }

                MediaCaptureInitializationSettings settings = new MediaCaptureInitializationSettings();
                var cameras = await DeviceInformation.FindAllAsync(DeviceClass.VideoCapture);
                var camera = cameras.FirstOrDefault();
                settings.VideoDeviceId = camera.Id;

                _media_capture = new MediaCapture();
                await _media_capture.InitializeAsync(settings);
                WebCam.Source = _media_capture;
            }

            if (_media_capture.CameraStreamState == Windows.Media.Devices.CameraStreamState.NotStreaming)
            {
                await _media_capture.StartPreviewAsync();
                WebCam.Visibility = Visibility.Visible;
            }
        }

        private bool processing;
        private Stopwatch watch;
        private int count;

        private async Task ProcessFrame()
        {
            if (processing)
            {
                // if we can't keep up to 30 fps, then ignore this tick.
                return;
            }
            try
            {
                if (watch == null)
                {
                    watch = new Stopwatch();
                    watch.Start();
                }

                processing = true;
                var frame = new VideoFrame(Windows.Graphics.Imaging.BitmapPixelFormat.Bgra8, (int)WebCam.Width, (int)WebCam.Height);
                await _media_capture.GetPreviewFrameAsync(frame);
                var results = await _model.EvaluateFrame(frame);
                await DrawBoxes(results, frame);
                count++;
                if (watch.ElapsedMilliseconds > 1000)
                {
                    ShowStatus(string.Format("{0} fps", count));
                    count = 0;
                    watch.Restart();
                }
            } 
            finally
            {
                processing = false;
            }
        }

        private float Sigmoid(float val)
        {
            var x = (float)Math.Exp(val);
            return x / (1.0f + x);
        }

        // draw bounding boxes on the output frame based on evaluation result
        private async Task DrawBoxes(List<Model.DetectionResult> detetions, VideoFrame frame)
        {

            this.OverlayCanvas.Children.Clear();
            for (int i=0; i < detetions.Count; ++i)
            {
                int top = (int)(detetions[i].bbox[0] * WebCam.Height);
                int left = (int)(detetions[i].bbox[1] * WebCam.Width);
                int bottom = (int)(detetions[i].bbox[2] * WebCam.Height);
                int right = (int)(detetions[i].bbox[3] * WebCam.Width);

                var brush = new ImageBrush();
                var bitmap_source = new SoftwareBitmapSource();
                await bitmap_source.SetBitmapAsync(frame.SoftwareBitmap);

                brush.ImageSource = bitmap_source;
                // brush.Stretch = Stretch.Fill;

                this.OverlayCanvas.Background = brush;

                var r = new Rectangle();
                r.Tag = i;
                r.Width = right - left;
                r.Height = bottom - top;
                r.Fill = this._fill_brush;
                r.Stroke = this._line_brush;
                r.StrokeThickness = this._line_thickness;
                r.Margin = new Thickness(left, top, 0, 0);

                this.OverlayCanvas.Children.Add(r);
                // Default configuration for border
                // Render text label
                

                var border = new Border();
                var backgroundColorBrush = new SolidColorBrush(Colors.Black);
                var foregroundColorBrush = new SolidColorBrush(Colors.SpringGreen);
                var textBlock = new TextBlock();
                textBlock.Foreground = foregroundColorBrush;
                textBlock.FontSize = 18;

                textBlock.Text = detetions[i].label;
                // Hide
                textBlock.Visibility = Visibility.Collapsed;
                border.Background = backgroundColorBrush;
                border.Child = textBlock;

                Canvas.SetLeft(border, detetions[i].bbox[1] * 416 + 2);
                Canvas.SetTop(border, detetions[i].bbox[0] * 416 + 2);
                textBlock.Visibility = Visibility.Visible;
                // Add to canvas
                this.OverlayCanvas.Children.Add(border);
            }
        }

        private void button_go_Click(object sender, RoutedEventArgs e)
        {
            if (_timer == null)
            {
                // now start processing frames, no need to do more than 30 per second!
                _timer = new DispatcherTimer()
                {
                    Interval = TimeSpan.FromMilliseconds(30)
                };
                _timer.Tick += OnTimerTick;
            }
        }

        void ShowStatus(string text)
        {
            textblock_status.Text = text;
        }


        private void OnTimerTick(object sender, object e)
        {
            // don't wait for this async task to finish
            _ = ProcessFrame();
        }
    }
}
