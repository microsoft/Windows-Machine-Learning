using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Windows.Media.Effects;
using Windows.Media.MediaProperties;
using Windows.Foundation.Collections;
using Windows.Graphics.DirectX.Direct3D11;
using Windows.Graphics.Imaging;
using System.Runtime.InteropServices;
using Windows.Media;
using Windows.AI.MachineLearning;
using Windows.Storage;
using Windows.Foundation;
using System.Runtime.InteropServices.WindowsRuntime;

namespace StyleTransferEffectComponent
{
    public sealed class StyleTransferVideoEffect : IBasicVideoEffect
    {

        private LearningModelSession _session;
        private LearningModelBinding _binding;
        string _inputImageDescription;
        string _outputImageDescription;

        public void Close(MediaEffectClosedReason reason)
        {
            // Dispose of effect resources
            if (_session != null) _session = null;
            if (_binding != null) _binding = null;
            if (_inputImageDescription != null) _inputImageDescription = null;
            if (_outputImageDescription != null) _outputImageDescription = null;
        }


        public void DiscardQueuedFrames() { }
        public bool IsReadOnly { get { return false; } }

        private VideoEncodingProperties encodingProperties;
        public void SetEncodingProperties(VideoEncodingProperties encodingProperties, IDirect3DDevice device)
        {
            this.encodingProperties = encodingProperties;
        }

        public IReadOnlyList<VideoEncodingProperties> SupportedEncodingProperties
        {
            get
            {
                var encodingProperties = new VideoEncodingProperties();
                encodingProperties.Subtype = "ARGB32";
                return new List<VideoEncodingProperties>() { encodingProperties };
            }
        }
        public MediaMemoryTypes SupportedMemoryTypes { get { return MediaMemoryTypes.GpuAndCpu; } }

        public bool TimeIndependent { get { return true; } }

        private IPropertySet configuration;
        public void SetProperties(IPropertySet configuration)
        {
            this.configuration = configuration;
        }

        public LearningModelBinding Binding
        {
            get
            {
                object val;
                if (configuration != null && configuration.TryGetValue("Binding", out val))
                {
                    return (LearningModelBinding)val;
                }
                return null;
            }
        }

        public LearningModelSession Session
        {
            get
            {
                object val;
                if (configuration != null && configuration.TryGetValue("Session", out val))
                {
                    return (LearningModelSession)val;
                }
                return null;
            }
        }

        public string InputImageDescription
        {
            get
            {
                object val;
                if (configuration != null && configuration.TryGetValue("InputImageDescription", out val))
                {
                    return (string)val;
                }
                return null;
            }
        }

        public string OutputImageDescription
        {
            get
            {
                object val;
                if (configuration != null && configuration.TryGetValue("OutputImageDescription", out val))
                {
                    return (string)val;
                }
                return null;
            }
        }


        public void ProcessFrame(ProcessVideoFrameContext context)
        {
            using (VideoFrame inputVideoFrame = context.InputFrame)
            using (VideoFrame outputVideoFrame = context.OutputFrame)
            {
                _binding = Binding;
                _session = Session;
                _inputImageDescription = InputImageDescription;
                _outputImageDescription = OutputImageDescription;

                Task.Run(async () =>
                {
                    VideoFrame inputTransformed = await CenterCropImageAsync(inputVideoFrame, 720, 720);
                    VideoFrame outputTransformed = new VideoFrame(BitmapPixelFormat.Bgra8, 720, 720);

                    _binding.Bind(_inputImageDescription, ImageFeatureValue.CreateFromVideoFrame(inputTransformed));
                    _binding.Bind(_outputImageDescription, ImageFeatureValue.CreateFromVideoFrame(outputTransformed));

                    var results = _session.Evaluate(_binding, "test");
                    await outputTransformed.CopyToAsync(outputVideoFrame);
                }).ConfigureAwait(false).GetAwaiter().GetResult();
            }
        }

        /// <summary>
        /// Crop image given a target width and height 
        /// </summary>
        /// <param name="inputVideoFrame"></param>
        /// <returns></returns>
        public static IAsyncOperation<VideoFrame> CenterCropImageAsync(VideoFrame inputVideoFrame, uint targetWidth, uint targetHeight)
        {
            return AsyncInfo.Run(async (token) =>
            {
                bool useDX = inputVideoFrame.SoftwareBitmap == null;
                VideoFrame result = null;
                // Center crop
                try
                {

                    // Since we will be center-cropping the image, figure which dimension has to be clipped
                    var frameHeight = useDX ? inputVideoFrame.Direct3DSurface.Description.Height : inputVideoFrame.SoftwareBitmap.PixelHeight;
                    var frameWidth = useDX ? inputVideoFrame.Direct3DSurface.Description.Width : inputVideoFrame.SoftwareBitmap.PixelWidth;

                    Rect cropRect = GetCropRect(frameWidth, frameHeight, targetWidth, targetHeight);
                    BitmapBounds cropBounds = new BitmapBounds()
                    {
                        Width = (uint)cropRect.Width,
                        Height = (uint)cropRect.Height,
                        X = (uint)cropRect.X,
                        Y = (uint)cropRect.Y
                    };

                    // Create the VideoFrame to be bound as input for evaluation
                    if (useDX)
                    {
                        if (inputVideoFrame.Direct3DSurface == null)
                        {
                            throw (new Exception("Invalid VideoFrame without SoftwareBitmap nor D3DSurface"));
                        }

                        result = new VideoFrame(BitmapPixelFormat.Bgra8,
                                                (int)targetWidth,
                                                (int)targetHeight,
                                                BitmapAlphaMode.Premultiplied);
                    }
                    else
                    {
                        result = new VideoFrame(BitmapPixelFormat.Bgra8,
                                                (int)targetWidth,
                                                (int)targetHeight,
                                                BitmapAlphaMode.Premultiplied);
                    }

                    await inputVideoFrame.CopyToAsync(result, cropBounds, null);
                }
                catch (Exception ex)
                {
                }

                return result;
            });
        }
        /// <summary>
        /// Calculate the center crop bounds given a set of source and target dimensions
        /// </summary>
        /// <param name="frameWidth"></param>
        /// <param name="frameHeight"></param>
        /// <param name="targetWidth"></param>
        /// <param name="targetHeight"></param>
        /// <returns></returns>
        public static Rect GetCropRect(int frameWidth, int frameHeight, uint targetWidth, uint targetHeight)
        {
            Rect rect = new Rect();

            // we need to recalculate the crop bounds in order to correctly center-crop the input image
            float flRequiredAspectRatio = (float)targetWidth / targetHeight;

            if (flRequiredAspectRatio * frameHeight > (float)frameWidth)
            {
                // clip on the y axis
                rect.Height = (uint)Math.Min((frameWidth / flRequiredAspectRatio + 0.5f), frameHeight);
                rect.Width = (uint)frameWidth;
                rect.X = 0;
                rect.Y = (uint)(frameHeight - rect.Height) / 2;
            }
            else // clip on the x axis
            {
                rect.Width = (uint)Math.Min((flRequiredAspectRatio * frameHeight + 0.5f), frameWidth);
                rect.Height = (uint)frameHeight;
                rect.X = (uint)(frameWidth - rect.Width) / 2; ;
                rect.Y = 0;
            }
            return rect;
        }
    }


}
