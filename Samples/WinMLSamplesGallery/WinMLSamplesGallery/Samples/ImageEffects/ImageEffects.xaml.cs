using Microsoft.AI.MachineLearning;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Media.Imaging;
using System;
using System.Diagnostics;
using System.Linq;
using System.Runtime.InteropServices;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Windows.Graphics.Imaging;
using Windows.Media;
using Windows.Storage;
using Windows.Storage.Pickers;
using Windows.UI;
using WinMLSamplesGallery.Common;

namespace WinMLSamplesGallery.Samples
{

    public enum Effect
    {
        NotSet = 0,
        Blur3x3,
        Blur5x5,
        Blur7x7,
        ResizeCubicFill,
        ResizeCubicFit,
        ResizeLinearFill,
        ResizeLinearFit,
        ResizeNearestFill,
        ResizeNearestFit,
        PixelSwizzle_123_321,
        PixelSwizzle_123_312,
        PixelSwizzle_123_213,
        PixelSwizzle_123_231,
        PixelSwizzle_123_132,
        MirrorHorizontal,
        MirrorVertical,
        RotateRight90,
        RotateLeft90,
        Sobel,
    }

    public sealed class EffectViewModel
    {
        public string Title { get; set; }
        public string Icon { get; set; }
        public string ModeIcon { get; set; }
        public Effect Tag { get; set; }
    }

    public sealed class PixelSwizzleViewModel
    {
        public string Title { get; set; }
        public Effect Tag { get; set; }
        public Color First { get; set; }
        public Color Second { get; set; }
        public Color Third { get; set; }
    }

    public sealed class OrientationViewModel
    {
        public string Title { get; set; }
        public Effect Tag { get; set; }
        public Transform Orientation { get; set; }
    }

#pragma warning disable CA1416 // Validate platform compatibility
    public static class LearningModelEvaluationResultExtensions
    {
        public static unsafe T Output<T>(this LearningModelEvaluationResult self, int index) where T : class
        {
            return self.Outputs.ElementAt(index).Value as T;
        }
        public static unsafe object Output(this LearningModelEvaluationResult self, int index)
        {
            return self.Outputs.ElementAt(index).Value as object;
        }

    }

#pragma warning restore CA1416 // Validate platform compatibility

    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class ImageEffects : Page
    {
        [DllImport("Kernel32.dll", CallingConvention = CallingConvention.Winapi)]
        private static extern void GetSystemTimePreciseAsFileTime(out long filetime);

        [DllImport("user32.dll", ExactSpelling = true, CharSet = CharSet.Auto, PreserveSig = true, SetLastError = false)]
        public static extern IntPtr GetActiveWindow();

        LearningModelDevice dmlDevice;
        LearningModelDevice cpuDevice;

        LearningModelSession tensorizationSession_ = null;
        LearningModelSession detensorizationSession_ = null;
        LearningModelSession resizeEffectSession_ = null;
        LearningModelSession orientationEffectSession_ = null;
        LearningModelSession pixelSwizzleEffectSession_ = null;
        LearningModelSession blurSharpenEffectSession_ = null;
        LearningModelSession contrastEffectSession_ = null;
        LearningModelSession artisticEffectsEffectSession_ = null;
        LearningModelSession shapeSession_ = null;

        private BitmapDecoder decoder_ = null;
        private uint currentImageWidth_;
        private uint currentImageHeight_;

        bool initialized_ = false;

        private bool IsCpu
        {
            get
            {
                return DeviceComboBox.SelectedIndex == 0;
            }
        }

        public ImageEffects()
        {
            this.InitializeComponent();

            dmlDevice = new LearningModelDevice(LearningModelDeviceKind.DirectX);
            cpuDevice = new LearningModelDevice(LearningModelDeviceKind.Cpu);

            BasicGridView.SelectedIndex = 0;

            InitializePicker(ResizeToggleSplitButton, ResizePicker, 2);
            InitializePicker(OrientationToggleSplitButton, OrientationPicker);
            InitializePicker(PixelSwizzleToggleSplitButton, PixelSwizzlePicker);
            InitializePicker(BlurSharpenToggleSplitButton, BlurSharpenPicker);
            InitializePicker(ArtisticEffectsToggleSplitButton, ArtisticEffectsPicker);

            ContrastMaxSlider.Value = .5;
            ContrastMinSlider.Value = .5;
            ContrastToggleSplitButton.IsChecked = false;

            initialized_ = true;
            ApplyEffects();
        }

        private void InitializePicker(ToggleSplitButton button, GridView picker, Nullable<int> index = null)
        {
            button.IsChecked = index.HasValue;
            picker.SelectedIndex = index.HasValue ? index.Value : 0;
            button.IsChecked = index.HasValue;
        }

#pragma warning disable CA1416 // Validate platform compatibility
        private void ApplyEffects(bool recreateSession = true)
        {
            if (!initialized_ || decoder_ == null)
            {
                return;
            }

            PerformanceMetricsMonitor.ClearLog();

            if (recreateSession)
            {
                RecreateSessions();
            }

            // TensorizeWithVideoFrame();

            long start, stop;

            // Tensorize
            start = HighResolutionClock.UtcNow();
            var pixelDataProvider = decoder_.GetPixelDataAsync().GetAwaiter().GetResult();
            var bytes = pixelDataProvider.DetachPixelData();
            var buffer = bytes.AsBuffer(); // Does this do a copy??
            var inputRawTensor = TensorUInt8Bit.CreateFromBuffer(new long[] { 1, buffer.Length }, buffer);
            // 3 channel NCHW
            var nextOutputShape = new long[] { 1, 3, currentImageHeight_, currentImageWidth_ };
            var intermediateTensor = TensorFloat.Create(nextOutputShape);
            var tensorizationBinding = Evaluate(tensorizationSession_, inputRawTensor, intermediateTensor);
            stop = HighResolutionClock.UtcNow();
            var tensorizationDuration = HighResolutionClock.DurationInMs(start, stop);

            // Resize
            start = HighResolutionClock.UtcNow();
            TensorFloat resizeOutput = null;
            LearningModelBinding resizeBinding = null;
            if (resizeEffectSession_ != null)
            {
                nextOutputShape = new long[] { 1, 3, 224, 224 };
                resizeOutput = TensorFloat.Create(nextOutputShape);
                resizeBinding = Evaluate(resizeEffectSession_, intermediateTensor, resizeOutput);
                intermediateTensor = resizeOutput;
            }
            stop = HighResolutionClock.UtcNow();
            var resizeDuration = HighResolutionClock.DurationInMs(start, stop);

            // Pixel Swizzle
            start = HighResolutionClock.UtcNow();
            TensorFloat swizzleOutput = null;
            LearningModelBinding swizzleBinding = null;
            if (pixelSwizzleEffectSession_ != null)
            {
                swizzleOutput = TensorFloat.Create(nextOutputShape);
                swizzleBinding = Evaluate(pixelSwizzleEffectSession_, intermediateTensor, swizzleOutput);
                intermediateTensor = swizzleOutput;
            }
            stop = HighResolutionClock.UtcNow();
            var swizzleDuration = HighResolutionClock.DurationInMs(start, stop);

            // Blur
            start = HighResolutionClock.UtcNow();
            TensorFloat blurOutput = null;
            LearningModelBinding blurBinding = null;
            if (blurSharpenEffectSession_ != null)
            {
                blurOutput = TensorFloat.Create(nextOutputShape);
                blurBinding = Evaluate(blurSharpenEffectSession_, intermediateTensor, blurOutput);
                intermediateTensor = blurOutput;
            }
            stop = HighResolutionClock.UtcNow();
            var blurDuration = HighResolutionClock.DurationInMs(start, stop);

            // Contrast
            start = HighResolutionClock.UtcNow();
            TensorFloat contrastOutput = null;
            LearningModelBinding contrastBinding = null;
            if (contrastEffectSession_ != null)
            {
                contrastOutput = TensorFloat.Create(nextOutputShape);
                contrastBinding = EvaluateContrastAndBrightnessSession(intermediateTensor, contrastOutput);
                intermediateTensor = contrastOutput;
            }
            stop = HighResolutionClock.UtcNow();
            var contrastDuration = HighResolutionClock.DurationInMs(start, stop);

            // Artistic Effects
            start = HighResolutionClock.UtcNow();
            LearningModelBinding artistiicEffectsBinding = null;
            if (artisticEffectsEffectSession_ != null)
            {
                var output = TensorFloat.Create(nextOutputShape);
                artistiicEffectsBinding = Evaluate(artisticEffectsEffectSession_, intermediateTensor, output);
                intermediateTensor = output;
            }
            stop = HighResolutionClock.UtcNow();
            var artisticEffectsDuration = HighResolutionClock.DurationInMs(start, stop);

            // Orientation
            start = HighResolutionClock.UtcNow();
            TensorFloat orientationOutput = null;
            LearningModelBinding orientationBinding = null;
            if (orientationEffectSession_ != null)
            {
                var orientationEffect = (OrientationPicker.SelectedItem as OrientationViewModel).Tag;
                if (orientationEffect == Effect.RotateLeft90 ||
                    orientationEffect == Effect.RotateRight90)
                {
                    nextOutputShape = new long[] { 1, 3, nextOutputShape[3], nextOutputShape[2] };
                    orientationOutput = TensorFloat.Create(nextOutputShape);
                }
                else
                {
                    orientationOutput = TensorFloat.Create(nextOutputShape);
                }
                orientationBinding = Evaluate(orientationEffectSession_, intermediateTensor, orientationOutput);
                intermediateTensor = orientationOutput;
            }
            stop = HighResolutionClock.UtcNow();
            var orientationDuration = HighResolutionClock.DurationInMs(start, stop);

            // Detensorize
            start = HighResolutionClock.UtcNow();
            var shape = intermediateTensor.Shape;
            var n = (int)shape[0];
            var c = (int)shape[1];
            var h = (int)shape[2];
            var w = (int)shape[3];

            // Rather than writing the data into the software bitmap ourselves from a Tensor (which may be on the gpu)
            // we call an indentity model to move the gpu memory back to the cpu via WinML de-tensorization.
            var outputImage = new SoftwareBitmap(BitmapPixelFormat.Bgra8, w, h, BitmapAlphaMode.Premultiplied);
            var outputFrame = VideoFrame.CreateWithSoftwareBitmap(outputImage);

            var descriptor = detensorizationSession_.Model.InputFeatures[0] as TensorFeatureDescriptor;
            var detensorizerShape = descriptor.Shape;
            if (c != detensorizerShape[1] || h != detensorizerShape[2] || w != detensorizerShape[3])
            {
                detensorizationSession_ = CreateLearningModelSession(TensorizationModels.IdentityNCHW(n, c, h, w));
            }
            var detensorizationBinding = Evaluate(detensorizationSession_, intermediateTensor, outputFrame, true);
            stop = HighResolutionClock.UtcNow();
            var detensorizationDuration = HighResolutionClock.DurationInMs(start, stop);

            // Render
            RenderImageInMainPanel(outputFrame);

            PerformanceMetricsMonitor.Log("Tensorize", tensorizationDuration);
            PerformanceMetricsMonitor.Log("Resize Effect", resizeDuration);
            PerformanceMetricsMonitor.Log("Swizzle Effect", swizzleDuration);
            PerformanceMetricsMonitor.Log("Blur Effect", blurDuration);
            PerformanceMetricsMonitor.Log("Contrast Effect", contrastDuration);
            PerformanceMetricsMonitor.Log("Artistic Effect", artisticEffectsDuration);
            PerformanceMetricsMonitor.Log("Orientation Effect", orientationDuration);
            PerformanceMetricsMonitor.Log("Detensorize", detensorizationDuration);
        }

        private LearningModelSession GetEffectSession(Effect effect)
        {
            switch (effect)
            {
                case Effect.Blur3x3:
                    return CreateLearningModelSession(TensorizationModels.AveragePool(3));
                case Effect.Blur5x5:
                    return CreateLearningModelSession(TensorizationModels.AveragePool(5));
                case Effect.Blur7x7:
                    return CreateLearningModelSession(TensorizationModels.AveragePool(7));
                case Effect.ResizeCubicFill:
                    return CreateLearningModelSession(TensorizationModels.UniformScaleAndCenterFill(currentImageHeight_, currentImageWidth_, 224, 224, "cubic"));
                case Effect.ResizeCubicFit:
                    return CreateLearningModelSession(TensorizationModels.UniformScaleAndCenterFit(currentImageHeight_, currentImageWidth_, 224, 224, "cubic"));
                case Effect.ResizeLinearFill:
                    return CreateLearningModelSession(TensorizationModels.UniformScaleAndCenterFill(currentImageHeight_, currentImageWidth_, 224, 224, "linear"));
                case Effect.ResizeLinearFit:
                    return CreateLearningModelSession(TensorizationModels.UniformScaleAndCenterFit(currentImageHeight_, currentImageWidth_, 224, 224, "linear"));
                case Effect.ResizeNearestFill:
                    return CreateLearningModelSession(TensorizationModels.UniformScaleAndCenterFill(currentImageHeight_, currentImageWidth_, 224, 224, "nearest"));
                case Effect.ResizeNearestFit:
                    return CreateLearningModelSession(TensorizationModels.UniformScaleAndCenterFit(currentImageHeight_, currentImageWidth_, 224, 224, "nearest"));
                case Effect.Sobel:
                    return CreateLearningModelSession(TensorizationModels.Sobel());
                case Effect.PixelSwizzle_123_321:
                    return CreateLearningModelSession(TensorizationModels.Swizzle(3, 2, 1));
                case Effect.PixelSwizzle_123_312:
                    return CreateLearningModelSession(TensorizationModels.Swizzle(3, 1, 2));
                case Effect.PixelSwizzle_123_213:
                    return CreateLearningModelSession(TensorizationModels.Swizzle(2, 1, 3));
                case Effect.PixelSwizzle_123_231:
                    return CreateLearningModelSession(TensorizationModels.Swizzle(2, 3, 1));
                case Effect.PixelSwizzle_123_132:
                    return CreateLearningModelSession(TensorizationModels.Swizzle(1, 3, 2));
                case Effect.MirrorHorizontal:
                    return CreateLearningModelSession(TensorizationModels.MirrorHorizontalNCHW());
                case Effect.MirrorVertical:
                    return CreateLearningModelSession(TensorizationModels.MirrorVerticalNCHW());
                case Effect.RotateRight90:
                    return CreateLearningModelSession(TensorizationModels.RotateRight90());
                case Effect.RotateLeft90:
                    return CreateLearningModelSession(TensorizationModels.RotateLeft90());
                default:
                    return null;
            }
        }

        private LearningModelSession CreateLearningModelSession(LearningModel model, bool closeModel = true)
        {
            var device = IsCpu ? cpuDevice : dmlDevice;
            var options = new LearningModelSessionOptions()
            {
                CloseModelOnSessionCreation = closeModel, // Close the model to prevent extra memory usage
                BatchSizeOverride = 0
            };
            var session = new LearningModelSession(model, device, options);
            return session;
        }

        private void RecreateSessions()
        {
            tensorizationSession_?.Dispose();
            tensorizationSession_ =
                CreateLearningModelSession(
                    TensorizationModels.ReshapeFlatBufferToNCHW(
                        1,
                        4,
                        currentImageHeight_,
                        currentImageWidth_));

            resizeEffectSession_?.Dispose();
            resizeEffectSession_ = GetEffect(ResizeToggleSplitButton, ResizePicker);

            pixelSwizzleEffectSession_?.Dispose();
            pixelSwizzleEffectSession_ = GetPixelSwizzleEffect();

            blurSharpenEffectSession_?.Dispose();
            blurSharpenEffectSession_ = GetEffect(BlurSharpenToggleSplitButton, BlurSharpenPicker);

            contrastEffectSession_?.Dispose();
            contrastEffectSession_ = ContrastToggleSplitButton.IsChecked ?
                                            CreateLearningModelSession(TensorizationModels.AdjustBrightnessAndContrast(
                                            1,
                                            3,
                                            resizeEffectSession_ != null ? 224 : currentImageHeight_,
                                            resizeEffectSession_ != null ? 224 : currentImageWidth_)) :
                                            null;

            artisticEffectsEffectSession_?.Dispose();
            artisticEffectsEffectSession_ = GetEffect(ArtisticEffectsToggleSplitButton, ArtisticEffectsPicker);

            orientationEffectSession_?.Dispose();
            orientationEffectSession_ = GetOrientationEffect();

            shapeSession_?.Dispose();
            shapeSession_ = CreateLearningModelSession(TensorizationModels.ShapeNCHW(1, 3, currentImageHeight_, currentImageWidth_));

            detensorizationSession_?.Dispose();
            detensorizationSession_ = CreateLearningModelSession(TensorizationModels.IdentityNCHW(
                                            1,
                                            3,
                                            resizeEffectSession_ != null ? 224 : currentImageHeight_,
                                            resizeEffectSession_ != null ? 224 : currentImageWidth_));
        }

        private LearningModelSession GetEffect(ToggleSplitButton effectToggleButton, GridView effectPicker)
        {
            if (effectToggleButton.IsChecked)
            {
                var effectViewModel = effectPicker.SelectedItem as EffectViewModel;
                return GetEffectSession(effectViewModel.Tag);
            }
            return null;
        }

        private LearningModelSession GetPixelSwizzleEffect()
        {
            if (PixelSwizzleToggleSplitButton.IsChecked)
            {
                var effectViewModel = PixelSwizzlePicker.SelectedItem as PixelSwizzleViewModel;
                return GetEffectSession(effectViewModel.Tag);
            }
            return null;
        }

        private LearningModelSession GetOrientationEffect()
        {
            if (OrientationToggleSplitButton.IsChecked)
            {
                var effectViewModel = OrientationPicker.SelectedItem as OrientationViewModel;
                return GetEffectSession(effectViewModel.Tag);
            }
            return null;
        }

        private TensorFloat TensorizeWithVideoFrame()
        {
            var s = new System.Diagnostics.Stopwatch();
            s.Start();
            var bitmap = decoder_.GetSoftwareBitmapAsync(BitmapPixelFormat.Bgra8, BitmapAlphaMode.Premultiplied).GetAwaiter().GetResult();
            var videoFrame = VideoFrame.CreateWithSoftwareBitmap(bitmap);
            var outputTensorI64 = TensorInt64Bit.Create();
            Evaluate(shapeSession_, videoFrame, outputTensorI64);
            s.Stop();
            PerformanceMetricsMonitor.Log("VideoFrame Tensorization", s.ElapsedMilliseconds * 1000000);
            return null;
        }

        private LearningModelBinding Evaluate(LearningModelSession session, object input, object output, bool wait = false)
        {
            // Create the binding
            var binding = new LearningModelBinding(session);

            // Bind inputs and outputs
            string inputName = session.Model.InputFeatures[0].Name;
            string outputName = session.Model.OutputFeatures[0].Name;
            binding.Bind(inputName, input);

            var outputBindProperties = new PropertySet();
            outputBindProperties.Add("DisableTensorCpuSync", PropertyValue.CreateBoolean(true));
            binding.Bind(outputName, output, outputBindProperties);

            // Evaluate
            EvaluateInternal(session, binding, wait);

            return binding;
        }

        private void EvaluateInternal(LearningModelSession session, LearningModelBinding binding, bool wait = false)
        {
            if (IsCpu)
            {
                session.Evaluate(binding, "");
            }
            else
            {
                var results = session.EvaluateAsync(binding, "");
                if (wait)
                {
                    results.GetAwaiter().GetResult();
                }
            }
        }

        private unsafe LearningModelBinding EvaluateContrastAndBrightnessSession(object input, object output)
        {
            var slope = Math.Tan(ContrastMaxSlider.Value * 3.14159 / 2);
            var yintercept = -255 * (ContrastMinSlider.Value * 2 - 1);

            if (yintercept < 0)
            {
                // it was the x-intercept
                yintercept = slope * yintercept;
            }

            var binding = new LearningModelBinding(contrastEffectSession_);
            binding.Bind("Input", input);
            binding.Bind("Slope", TensorFloat.CreateFromArray(new long[] { 1 }, new float[] { (float)slope }));
            binding.Bind("YIntercept", TensorFloat.CreateFromArray(new long[] { 1 }, new float[] { (float)yintercept }));

            var outputBindProperties = new PropertySet();
            outputBindProperties.Add("DisableTensorCpuSync", PropertyValue.CreateBoolean(true));
            binding.Bind("Output", output, outputBindProperties);

            EvaluateInternal(contrastEffectSession_, binding);

            return binding;
        }

#pragma warning restore CA1416 // Validate platform compatibility

        private void OpenButton_Clicked(object sender, RoutedEventArgs e)
        {
            var file = File.PickImageFiles();
            if (file != null)
            {
                BasicGridView.SelectedItem = null;
                decoder_ = CreateBitmapDecoderFromFile(file);
                ApplyEffects();
            }
        }

        private void SampleInputsGridView_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            var gridView = sender as GridView;
            var thumbnail = gridView.SelectedItem as WinMLSamplesGallery.Controls.Thumbnail;
            if (thumbnail != null)
            {
                var image = thumbnail.ImageUri;
                var file = StorageFile.GetFileFromApplicationUriAsync(new Uri(image)).GetAwaiter().GetResult();
                decoder_ = CreateBitmapDecoderFromFile(file);
                ApplyEffects();
            }
        }

        private void RenderImageInMainPanel(VideoFrame videoFrame)
        {
            if (videoFrame != null)
            {
                SoftwareBitmap displayBitmap = videoFrame.SoftwareBitmap;
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
        }


        private BitmapDecoder CreateBitmapDecoderFromFile(StorageFile file)
        {
            var stream = file.OpenAsync(FileAccessMode.Read).GetAwaiter().GetResult();
            var decoder =  BitmapDecoder.CreateAsync(stream).GetAwaiter().GetResult();
            currentImageWidth_ = decoder.PixelWidth;
            currentImageHeight_ = decoder.PixelHeight;
            return decoder;
        }

        private void IsCheckedChanged(ToggleSplitButton sender, ToggleSplitButtonIsCheckedChangedEventArgs args)
        {
            ApplyEffects();
        }

        private void ResizePicker_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            CompleteFlyoutSelection(ResizeToggleSplitButton, false);
            ApplyEffects();
        }

        private void OrientationPicker_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            CompleteFlyoutSelection(OrientationToggleSplitButton, false);
            ApplyEffects();
        }

        private void PixelSwizzlePicker_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            CompleteFlyoutSelection(PixelSwizzleToggleSplitButton, false);
            ApplyEffects();
        }

        private void BlurSharpenPicker_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            CompleteFlyoutSelection(BlurSharpenToggleSplitButton, false);
            ApplyEffects();
        }


        private void ContrastMaxSlider_ValueChanged(object sender, Microsoft.UI.Xaml.Controls.Primitives.RangeBaseValueChangedEventArgs e)
        {
            CompleteFlyoutSelection(ContrastToggleSplitButton, false);
            ApplyEffects(false);
        }
        private void ContrastMinSlider_ValueChanged(object sender, Microsoft.UI.Xaml.Controls.Primitives.RangeBaseValueChangedEventArgs e)
        {
            CompleteFlyoutSelection(ContrastToggleSplitButton, false);
            ApplyEffects(false);
        }

        private void ArtisticEffectsPicker_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            CompleteFlyoutSelection(ArtisticEffectsToggleSplitButton);
            ApplyEffects();
        }

        private void CompleteFlyoutSelection(ToggleSplitButton button, bool hideFlyout = true)
        {
            button.IsChecked = true;
            if (hideFlyout)
            {
                button.Flyout.Hide();
            }
        }

        private void DeviceComboBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            ApplyEffects();
        }
    }
}
/*
  // Will need this for 0 copy interop
    public class PinnedBuffer : Windows.Storage.Streams.IBuffer, IBufferByteAccess


    {
        GCHandle handle_;
        IntPtr ptr_;
        uint num_bytes_;

        public PinnedBuffer(byte[] array) {
            num_bytes_ = (uint)array.Length;
            handle_ = GCHandle.Alloc(array, GCHandleType.Pinned);
            ptr_ = GCHandle.ToIntPtr(handle_);
        }

        ~PinnedBuffer() {
            handle_.Free();
        }

        public uint Capacity => num_bytes_;

        public uint Length
        {
            get => num_bytes_;
            set
            {
                num_bytes_ = value;
            }
        }

#pragma warning disable CA1416 // Validate platform compatibility
        public TensorFloat ToTensorFloat()
        {
            return TensorFloat.CreateFromBuffer(new long[] { num_bytes_ }, this);
        }

        public unsafe void Buffer(out byte* pByte)
        {
            pByte = (byte*)ptr_;
        }
#pragma warning restore CA1416 // Validate platform compatibility
    }
 */