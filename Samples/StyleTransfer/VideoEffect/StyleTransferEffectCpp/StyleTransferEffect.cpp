#include "pch.h"
#include "StyleTransferEffect.h"
#include "StyleTransferEffect.g.cpp"


namespace winrt::StyleTransferEffectCpp::implementation
{
	StyleTransferEffect::StyleTransferEffect() : outputTransformed(VideoFrame(Windows::Graphics::Imaging::BitmapPixelFormat::Bgra8, 720, 720)),
		Session(nullptr),
		Binding(nullptr)
	{
	}

	IVectorView<VideoEncodingProperties> StyleTransferEffect::SupportedEncodingProperties() {
		VideoEncodingProperties encodingProperties = VideoEncodingProperties();
		encodingProperties.Subtype(L"ARGB32");
		return single_threaded_vector(std::move(std::vector<VideoEncodingProperties>{encodingProperties})).GetView();
	}

	bool StyleTransferEffect::TimeIndependent() { return true; }
	MediaMemoryTypes StyleTransferEffect::SupportedMemoryTypes() { return MediaMemoryTypes::GpuAndCpu; }
	bool StyleTransferEffect::IsReadOnly() { return false; }
	void StyleTransferEffect::DiscardQueuedFrames() {}

	void StyleTransferEffect::Close(MediaEffectClosedReason) {
		OutputDebugString(L"Close Begin | ");
		Processing.lock();
		OutputDebugString(L"Close\n");
		if (Binding != nullptr) Binding.Clear();
		if (Session != nullptr) Session.Close();
		outputTransformed.Close();
		Processing.unlock();
	}

	void StyleTransferEffect::ProcessFrame(ProcessVideoFrameContext context) {
		Processing.lock();
		OutputDebugString(L"Start ProcessFrame | ");
		VideoFrame inputFrame = context.InputFrame();
		VideoFrame outputFrame = context.OutputFrame();

		OutputDebugString(L"PF Binding | ");
		Binding.Bind(InputImageDescription, inputFrame);
		Binding.Bind(OutputImageDescription, outputTransformed);

		OutputDebugString(L"PF Eval | ");
		Session.Evaluate(Binding, L"test");
		outputTransformed.CopyToAsync(context.OutputFrame());
		OutputDebugString(L"Stop ProcessFrame | ");
		Processing.unlock();
		OutputDebugString(L"End Lock ProcessFrame\n");
	}

	void StyleTransferEffect::SetEncodingProperties(VideoEncodingProperties props, IDirect3DDevice device) {
		encodingProperties = props;
	}

	void StyleTransferEffect::SetProperties(IPropertySet config) {
		this->configuration = config;

		IInspectable val = configuration.TryLookup(L"Session");
		if (val) {
			Session = val.try_as<LearningModelSession>();
		}
		val = configuration.TryLookup(L"Binding");
		if (val) {
			Binding = val.try_as<LearningModelBinding>();
		}
		val = configuration.TryLookup(L"InputImageDescription");
		if (val) {
			InputImageDescription = unbox_value<hstring>(val);
		}
		val = configuration.TryLookup(L"OutputImageDescription");
		if (val) {
			OutputImageDescription = unbox_value<hstring>(val);
		}
	}
}
