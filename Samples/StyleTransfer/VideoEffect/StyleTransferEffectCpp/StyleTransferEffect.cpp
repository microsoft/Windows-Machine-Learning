#include "pch.h"
#include "StyleTransferEffect.h"
#include "StyleTransferEffect.g.cpp"
#include <ppltasks.h>
#include <sstream>

using namespace std;
using namespace winrt::Windows::Storage;
using namespace winrt::Windows::Storage::Streams;
using namespace concurrency;

namespace winrt::StyleTransferEffectCpp::implementation
{
	StyleTransferEffect::StyleTransferEffect() :
		cachedOutput(nullptr),
		cachedOutputCopy(VideoFrame(Windows::Graphics::Imaging::BitmapPixelFormat::Bgra8, 640, 360)),
		outputTransformed(VideoFrame(Windows::Graphics::Imaging::BitmapPixelFormat::Bgra8, 720, 720)),
		Session(nullptr),
		Binding(nullptr)
	{

	}

	IVectorView<VideoEncodingProperties> StyleTransferEffect::SupportedEncodingProperties() {
		VideoEncodingProperties encodingProperties = VideoEncodingProperties();
		encodingProperties.Subtype(L"ARGB32");
		return single_threaded_vector(std::vector<VideoEncodingProperties>{encodingProperties}).GetView();
	}

	bool StyleTransferEffect::TimeIndependent() { return true; }
	MediaMemoryTypes StyleTransferEffect::SupportedMemoryTypes() { return MediaMemoryTypes::GpuAndCpu; }
	bool StyleTransferEffect::IsReadOnly() { return false; }
	void StyleTransferEffect::DiscardQueuedFrames() {}

	void StyleTransferEffect::Close(MediaEffectClosedReason m) {
		OutputDebugString(L"Close Begin | ");
		std::lock_guard<mutex> guard{ Processing };
		OutputDebugString(L"Close\n");
		if (Binding != nullptr) Binding.Clear();
		if (Session != nullptr) Session.Close();
	}

	std::wstring index(const std::thread::id id)
	{
		static std::size_t nextindex = 0;
		static std::mutex my_mutex;
		static std::map<std::thread::id, std::string> ids;
		std::lock_guard<std::mutex> lock(my_mutex);
		if (ids.find(id) == ids.end()) {
			ids[id] = std::to_string((int)nextindex);
			nextindex++;
		}
		return std::wstring(ids[id].begin(), ids[id].end());
	}

	void StyleTransferEffect::ProcessFrame(ProcessVideoFrameContext context) {

		OutputDebugString(L"PF Start | ");
		//OutputDebugString(index(thread().get_id()).c_str());
		auto now = std::chrono::high_resolution_clock::now();
		VideoFrame inputFrame = context.InputFrame();
		VideoFrame outputFrame = context.OutputFrame();
		VideoFrame temp = cachedOutput;

		OutputDebugString(L"PF Eval | ");
		if (evalStatus == nullptr || evalStatus.Status() != Windows::Foundation::AsyncStatus::Started)
		{
			Binding.Bind(InputImageDescription, inputFrame);
			Binding.Bind(OutputImageDescription, outputTransformed);
			auto nowEval = std::chrono::high_resolution_clock::now();
			evalStatus = Session.EvaluateAsync(Binding, L"test");
			evalStatus.Completed([&, nowEval](auto&& asyncInfo, winrt::Windows::Foundation::AsyncStatus const args) {
				VideoFrame output = asyncInfo.GetResults().Outputs().Lookup(OutputImageDescription).try_as<VideoFrame>();
				OutputDebugString(L"PF Copy | ");
				output.CopyToAsync(cachedOutputCopy);
				{
					cachedOutput = cachedOutputCopy;
				}
				OutputDebugString(L"PF End\n ");
				auto timePassedEval = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - nowEval);
				std::wostringstream ss;
				ss << (timePassedEval.count() / 1000.f);
				Notifier.SetFrameRate(timePassedEval.count() / 1000.f);
				OutputDebugString(L"\nEval Time : ");
				OutputDebugString(ss.str().c_str());
				});

		}
		if (temp != nullptr) {
			OutputDebugString(L"\nStart CopyAsync | ");
			temp.CopyToAsync(context.OutputFrame()).get();
			OutputDebugString(L"Stop CopyAsync\n");
		}

		auto timePassed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - now);
		//Notifier.SetFrameRate(1000.f / timePassed.count()); // Convert to FPS: milli to seconds, invert

	}

	void StyleTransferEffect::SetEncodingProperties(VideoEncodingProperties props, IDirect3DDevice device) {
		encodingProperties = props;
	}

	void StyleTransferEffect::SetProperties(IPropertySet config) {
		this->configuration = config;
		hstring modelName;
		bool useGpu;

		IInspectable val = config.TryLookup(L"ModelName");
		if (val) modelName = unbox_value<hstring>(val);
		else winrt::throw_hresult(E_FAIL);
		val = configuration.TryLookup(L"UseGpu");
		if (val) useGpu = unbox_value<bool>(val);
		else winrt::throw_hresult(E_FAIL);
		val = configuration.TryLookup(L"Notifier");
		if (val) Notifier = val.try_as<StyleTransferEffectNotifier>();
		else winrt::throw_hresult(E_FAIL);

		LearningModel m_model = LearningModel::LoadFromFilePath(modelName);
		LearningModelDeviceKind m_device = useGpu ? LearningModelDeviceKind::DirectX : LearningModelDeviceKind::Cpu;
		Session = LearningModelSession{ m_model, LearningModelDevice(m_device) };
		Binding = LearningModelBinding{ Session };

		InputImageDescription = L"inputImage";
		OutputImageDescription = L"outputImage";
	}
}
