#include "pch.h"
#include "Class.h"
#include "Class.g.cpp"


namespace winrt::RuntimeComponent2::implementation
{
	Class::Class() : outputTransformed(VideoFrame(Windows::Graphics::Imaging::BitmapPixelFormat::Bgra8, 720, 720)) {}

	IVectorView<VideoEncodingProperties> Class::SupportedEncodingProperties() {
		VideoEncodingProperties encodingProperties = VideoEncodingProperties();
		encodingProperties.Subtype(L"ARGB32");
		return single_threaded_vector(std::move(std::vector<VideoEncodingProperties>{encodingProperties})).GetView();
	}

	LearningModelSession Class::Session() {
		if (this->configuration) {
			IInspectable val = configuration.TryLookup(L"Session");
			if (val) {
				return val.try_as<LearningModelSession>();
			}
		}
		return NULL;
	}
	LearningModelBinding Class::Binding() {
		if (configuration) {
			IInspectable val = configuration.TryLookup(L"Binding");
			if (val) {
				return val.try_as<LearningModelBinding>();
			}
		}
		return NULL;
	}

	hstring Class::InputImageDescription() {
		if (configuration) {
			IInspectable val = configuration.TryLookup(L"InputImageDescription");
			if (val) {
				return unbox_value<hstring>(val);
			}
		}
		return L"";
	}
	hstring Class::OutputImageDescription() {
		if (configuration) {
			IInspectable val = configuration.TryLookup(L"OutputImageDescription");
			if (val) {
				return unbox_value<hstring>(val);
			}
		}
		return L"";
	}


	bool Class::TimeIndependent() { return true; }
	MediaMemoryTypes Class::SupportedMemoryTypes() { return MediaMemoryTypes::GpuAndCpu; }
	bool Class::IsReadOnly() { return false; }
	void Class::DiscardQueuedFrames() {}

	void Class::Close(MediaEffectClosedReason) {
		throw hresult_not_implemented();
	}


	void Class::ProcessFrame(ProcessVideoFrameContext context) {
		LearningModelSession _session = Session();
		LearningModelBinding _binding = Binding();
		VideoFrame inputFrame = context.InputFrame();
		VideoFrame outputFrame = context.OutputFrame();


		_binding.Bind(InputImageDescription(), inputFrame);
		_binding.Bind(OutputImageDescription(), outputTransformed);

		_session.Evaluate(_binding, L"test");
		outputTransformed.CopyToAsync(context.OutputFrame());
	}

	void Class::SetEncodingProperties(VideoEncodingProperties props, IDirect3DDevice device) {
		encodingProperties = props;
	}

	void Class::SetProperties(IPropertySet config) {
		this->configuration = config;
	}
}
