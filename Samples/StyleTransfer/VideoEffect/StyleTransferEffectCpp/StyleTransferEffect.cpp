#include "pch.h"
#include "StyleTransferEffect.h"
#include "StyleTransferEffect.g.cpp"
using namespace std;
using namespace winrt::Windows::Storage;
using namespace winrt::Windows::Storage::Streams;

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
		OutputDebugString(L"Start ProcessFrame | ");
		auto startSync = std::chrono::high_resolution_clock::now();

		VideoFrame inputFrame = context.InputFrame();
		VideoFrame outputFrame = context.OutputFrame();

		Processing.lock();
		OutputDebugString(L"PF Locked | ");
		Binding.Bind(InputImageDescription, inputFrame);
		Binding.Bind(OutputImageDescription, outputTransformed);

		OutputDebugString(L"PF Eval | ");
		Session.Evaluate(Binding, L"test");
		outputTransformed.CopyToAsync(context.OutputFrame());
		Processing.unlock();
		OutputDebugString(L"PF Unlocked");

		auto syncTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - startSync);
		Notifier.SetFrameRate(1000.f / syncTime.count()); // Convert to FPS: milli to seconds, invert 
	}

	void StyleTransferEffect::SetEncodingProperties(VideoEncodingProperties props, IDirect3DDevice device) {
		encodingProperties = props;
	}

	void StyleTransferEffect::SetProperties(IPropertySet config) {
		this->configuration = config;
		hstring modelName;
		IInspectable val = config.TryLookup(L"ModelName");
		if (!val) {
			return;
		}
		modelName = unbox_value<hstring>(val);
		val = configuration.TryLookup(L"UseGPU");
		bool useGpu = unbox_value<bool>(val);
		val = configuration.TryLookup(L"Notifier");
		Notifier = val.try_as<StyleTransferEffectNotifier>();

		LearningModel m_model = LearningModel::LoadFromFilePath(modelName);
		LearningModelDeviceKind m_device = useGpu ? LearningModelDeviceKind::DirectX : LearningModelDeviceKind::Cpu;
		Session = LearningModelSession{ m_model, LearningModelDevice(m_device) };
		Binding = LearningModelBinding{ Session };

		InputImageDescription = L"inputImage";
		OutputImageDescription = L"outputImage";
	}
}
