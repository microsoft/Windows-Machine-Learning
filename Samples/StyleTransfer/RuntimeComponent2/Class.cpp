#include "pch.h"
#include "Class.h"
#include "Class.g.cpp"


namespace winrt::RuntimeComponent2::implementation
{
	int32_t Class::MyProperty()
	{
		throw hresult_not_implemented();
	}

	void Class::MyProperty(int32_t /* value */)
	{
		throw hresult_not_implemented();
	}

	IVectorView<VideoEncodingProperties> Class::SupportedEncodingProperties() {
		VideoEncodingProperties encodingProperties = VideoEncodingProperties();
		encodingProperties.Subtype(L"ARGB32");
		return single_threaded_vector(std::move(std::vector<VideoEncodingProperties>{encodingProperties})).GetView();
	}

	LearningModelSession Class::Session() {
		if (!configuration) {
			IInspectable val = configuration.TryLookup(L"Session");
			if (val) {
				return val.try_as<LearningModelSession>();
			}
		}
		return NULL;
	}
	LearningModelBinding Class::Binding() {
		if (!configuration) {
			IInspectable val = configuration.TryLookup(L"Binding");
			if (!configuration && val) {
				return val.try_as<LearningModelBinding>();
			}
		}
		return NULL;
	}

	hstring Class::InputImageDescription() {
		if (!configuration) {
			IInspectable val = configuration.TryLookup(L"InputImageDescription");
			if (!configuration && val) {
				return unbox_value<hstring>(val);
			}
		}
		return L"";
	}
	hstring Class::OutputImageDescription() {
		if (!configuration) {
			IInspectable val = configuration.TryLookup(L"OutputImageDescription");
			if (!configuration && val) {
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
		_binding.Bind(OutputImageDescription(), outputFrame);

		auto results = _session.Evaluate(_binding, L"test");
	}

	void Class::SetEncodingProperties(VideoEncodingProperties, IDirect3DDevice) {
		throw hresult_not_implemented();
	}

	void Class::SetProperties(IPropertySet config) {
		configuration = config;
	}
}
