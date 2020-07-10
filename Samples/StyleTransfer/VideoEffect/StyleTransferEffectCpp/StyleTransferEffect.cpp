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
		//Processing.lock();
		this->configuration = config;
		hstring modelName;
		IInspectable val = config.TryLookup(L"ModelName");
		if (!val) {
			return;
		}
		modelName = unbox_value<hstring>(val);
		val = configuration.TryLookup(L"UseGPU");
		bool useGpu = unbox_value<bool>(val);
		OutputDebugString(modelName.c_str());
		LearningModel m_model = LearningModel::LoadFromFilePath(modelName);

		LearningModelDeviceKind m_device = useGpu ? LearningModelDeviceKind::DirectX : LearningModelDeviceKind::Cpu;
		Session = LearningModelSession{ m_model, LearningModelDevice(m_device) };
		Binding = LearningModelBinding{ Session };

		InputImageDescription = L"inputImage";
		OutputImageDescription = L"outputImage";
		//Processing.unlock();
	}
}
