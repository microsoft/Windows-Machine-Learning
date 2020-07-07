#include "pch.h"
#include "StyleTransferEffect.h"
#include "StyleTransferEffect.g.cpp"


namespace winrt::StyleTransferEffectCpp::implementation
{
	StyleTransferEffect::StyleTransferEffect() : outputTransformed(VideoFrame(Windows::Graphics::Imaging::BitmapPixelFormat::Bgra8, 720, 720)) {
	}

	IVectorView<VideoEncodingProperties> StyleTransferEffect::SupportedEncodingProperties() {
		VideoEncodingProperties encodingProperties = VideoEncodingProperties();
		encodingProperties.Subtype(L"ARGB32");
		return single_threaded_vector(std::move(std::vector<VideoEncodingProperties>{encodingProperties})).GetView();
	}

	LearningModelSession StyleTransferEffect::Session() {
		if (this->configuration) {
			IInspectable val = configuration.TryLookup(L"Session");
			if (val) {
				return val.try_as<LearningModelSession>();
			}
		}
		return NULL;
	}
	LearningModelBinding StyleTransferEffect::Binding() {
		if (configuration) {
			IInspectable val = configuration.TryLookup(L"Binding");
			if (val) {
				return val.try_as<LearningModelBinding>();
			}
		}
		return NULL;
	}

	hstring StyleTransferEffect::InputImageDescription() {
		if (configuration) {
			IInspectable val = configuration.TryLookup(L"InputImageDescription");
			if (val) {
				return unbox_value<hstring>(val);
			}
		}
		return L"";
	}
	hstring StyleTransferEffect::OutputImageDescription() {
		if (configuration) {
			IInspectable val = configuration.TryLookup(L"OutputImageDescription");
			if (val) {
				return unbox_value<hstring>(val);
			}
		}
		return L"";
	}


	bool StyleTransferEffect::TimeIndependent() { return true; }
	MediaMemoryTypes StyleTransferEffect::SupportedMemoryTypes() { return MediaMemoryTypes::GpuAndCpu; }
	bool StyleTransferEffect::IsReadOnly() { return false; }
	void StyleTransferEffect::DiscardQueuedFrames() {}

	void StyleTransferEffect::Close(MediaEffectClosedReason) {
	}


	void StyleTransferEffect::ProcessFrame(ProcessVideoFrameContext context) {
		LearningModelSession _session = Session();
		LearningModelBinding _binding = Binding();
		VideoFrame inputFrame = context.InputFrame();
		VideoFrame outputFrame = context.OutputFrame();


		_binding.Bind(InputImageDescription(), inputFrame);
		_binding.Bind(OutputImageDescription(), outputTransformed);

		_session.Evaluate(_binding, L"test");
		outputTransformed.CopyToAsync(context.OutputFrame());
	}

	void StyleTransferEffect::SetEncodingProperties(VideoEncodingProperties props, IDirect3DDevice device) {
		encodingProperties = props;
	}

	void StyleTransferEffect::SetProperties(IPropertySet config) {
		this->configuration = config;

	}
}
