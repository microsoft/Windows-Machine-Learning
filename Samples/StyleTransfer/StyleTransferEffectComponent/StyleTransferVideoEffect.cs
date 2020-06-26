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

                VideoFrame inputTransformed = new VideoFrame(BitmapPixelFormat.Bgra8, 720, 720);
                Task.Run(async () =>
                {
                    await inputVideoFrame.CopyToAsync(inputTransformed);

                    VideoFrame outputTransformed = new VideoFrame(BitmapPixelFormat.Bgra8, 720, 720);
                    await inputVideoFrame.CopyToAsync(inputTransformed);

                    _binding.Bind(_inputImageDescription, ImageFeatureValue.CreateFromVideoFrame(inputTransformed));
                    _binding.Bind(_outputImageDescription, ImageFeatureValue.CreateFromVideoFrame(outputTransformed));

                    var results = _session.Evaluate(_binding, "test");
                    await outputTransformed.CopyToAsync(outputVideoFrame);
                }).ConfigureAwait(false).GetAwaiter().GetResult();


            }
        }
    }
}
