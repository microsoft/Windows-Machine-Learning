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
		for (int i = 0; i < swapChainEntryCount; i++)
		{
			bindings.push_back(std::make_unique<SwapChainEntry>());
		}
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
		// Make sure evalAsyncs are done before clearing resources
		for (int i = 0; i < swapChainEntryCount; i++) {
			if (bindings[i]->activetask != nullptr &&
				bindings[i]->binding != nullptr)
			{
				bindings[i]->activetask.get();
				bindings[i]->binding.Clear();
			}
		}
	}

	void StyleTransferEffect::SubmitEval(VideoFrame input, VideoFrame output) {
		auto currentBinding = bindings[0].get();
		//VideoFrame outputTransformed = cachedOutput;
		// Different way of waiting for a swapchain index to finish? 
		// Or would it be just setting the output to be a cached frame? 
		if (currentBinding->activetask == nullptr
			|| currentBinding->activetask.Status() != Windows::Foundation::AsyncStatus::Started)
		{
			auto now = std::chrono::high_resolution_clock::now();
			OutputDebugString(L"PF Start new Eval ");
			std::wostringstream ss;
			ss << swapChainIndex;
			OutputDebugString(ss.str().c_str());
			OutputDebugString(L" | ");

			// bind the input and the output buffers by name
			currentBinding->binding.Bind(InputImageDescription, input);
			// submit an eval and wait for it to finish submitting work
			{
				std::lock_guard<mutex> guard{ Processing };
				std::rotate(bindings.begin(), bindings.begin() + 1, bindings.end());
				finishedIdx = (finishedIdx - 1 + swapChainEntryCount) % swapChainEntryCount;
			}
			currentBinding->activetask = Session.EvaluateAsync(currentBinding->binding, ss.str().c_str());
			currentBinding->activetask.Completed([&, currentBinding, now](auto&& asyncInfo, winrt::Windows::Foundation::AsyncStatus const args) {
				OutputDebugString(L"PF Eval completed |");
				VideoFrame evalOutput = asyncInfo.GetResults().Outputs().Lookup(OutputImageDescription).try_as<VideoFrame>();
				int bindingIdx;
				bool finishedFrameUpdated;
				{
					std::lock_guard<mutex> guard{ Processing };
					auto binding = std::find_if(bindings.begin(), bindings.end(), [currentBinding](const auto& b) {return b.get() == currentBinding; });
					bindingIdx = std::distance(bindings.begin(), binding);
					finishedFrameUpdated = bindingIdx >= finishedIdx;
					finishedIdx = finishedFrameUpdated ? bindingIdx : finishedIdx;

				}
				if (finishedFrameUpdated)
				{
					OutputDebugString(L"PF Copy | ");
					evalOutput.CopyToAsync(currentBinding->outputCache);
				}

				auto timePassed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - now);
				Notifier.SetFrameRate(1000.f / timePassed.count()); // Convert to FPS: milli to seconds, invert
				OutputDebugString(L"PF End ");
				});
		}
		if (bindings[finishedIdx]->outputCache != nullptr) {
			std::wostringstream ss;
			ss << finishedIdx;
			//std::lock_guard<mutex> guard{ Copy };
			OutputDebugString(L"\nStart CopyAsync ");
			OutputDebugString(ss.str().c_str());
			bindings[finishedIdx]->outputCache.CopyToAsync(output).get();
			OutputDebugString(L" | Stop CopyAsync\n");
		}
		// return without waiting for the submit to finish, setup the completion handler
	}

	void StyleTransferEffect::ProcessFrame(ProcessVideoFrameContext context) {
		OutputDebugString(L"PF Start | ");
		auto now = std::chrono::high_resolution_clock::now();
		VideoFrame inputFrame = context.InputFrame();
		VideoFrame outputFrame = context.OutputFrame();

		SubmitEval(inputFrame, outputFrame);

		swapChainIndex = (++swapChainIndex) % swapChainEntryCount; // move on to the next entry after each call to PF. 
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

		// Create set of bindings to cycle through
		for (int i = 0; i < swapChainEntryCount; i++) {
			bindings[i]->binding = LearningModelBinding(Session);
			bindings[i]->binding.Bind(OutputImageDescription, VideoFrame(Windows::Graphics::Imaging::BitmapPixelFormat::Bgra8, 720, 720));
		}
	}
}
