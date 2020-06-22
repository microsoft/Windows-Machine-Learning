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
            if (_session != null) { _session = null; }
            if (_binding != null) { _binding = null; }
            if (_inputImageDescription != null) { _inputImageDescription = null; }
            if (_outputImageDescription != null) { _outputImageDescription = null; }
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

                // If the list is empty, the encoding type will be ARGB32.
                // return new List<VideoEncodingProperties>();
            }
        }
        // TODO: Chagne to GpuAndCpu, or based on toggle value in UI
        public MediaMemoryTypes SupportedMemoryTypes { get { return MediaMemoryTypes.Cpu; } }

        public bool TimeIndependent { get { return true; } }

        private IPropertySet configuration;
        public void SetProperties(IPropertySet configuration)
        {
            this.configuration = configuration;
        }

        public LearningModelBinding Binding
        {
            get
            {   // TODO: Read in model from configuration
                // If null, then fail. 
                object val;
                if (configuration != null && configuration.TryGetValue("Binding", out val))
                {
                    return (LearningModelBinding)val;
                }
                return null; // Different default value/ raise exception
            }
        }

        public LearningModelSession Session
        {
            get
            {   // TODO: Read in model from configuration
                // If null, then fail. 
                object val;
                if (configuration != null && configuration.TryGetValue("Session", out val))
                {
                    return (LearningModelSession)val;
                }
                return null; // Different default value/ raise exception
            }
        }

        public string InputImageDescription
        {
            get
            {   // TODO: Read in model from configuration
                // If null, then fail. 
                object val;
                if (configuration != null && configuration.TryGetValue("InputImageDescription", out val))
                {
                    return (string)val;
                }
                return null; // Different default value/ raise exception
            }
        }

        public string OutputImageDescription
        {
            get
            {   // TODO: Read in model from configuration
                // If null, then fail. 
                object val;
                if (configuration != null && configuration.TryGetValue("OutputImageDescription", out val))
                {
                    return (string)val;
                }
                return null; // Different default value/ raise exception
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

                _binding.Bind(_inputImageDescription, ImageFeatureValue.CreateFromVideoFrame(inputVideoFrame));
                _binding.Bind(_outputImageDescription, ImageFeatureValue.CreateFromVideoFrame(outputVideoFrame)); // Check if this correctly sets the context output

                var results = _session.Evaluate(_binding, "test");
            }
        }
    }
}
