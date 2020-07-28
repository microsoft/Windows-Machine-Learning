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
		if (bindings != nullptr) {
			for (int i = 0; i < swapChainEntryCount; i++) {
				bindings[i].binding.Clear();
			}
		}
	}

	void StyleTransferEffect::SubmitEval(int swapchaindex, VideoFrame input, VideoFrame output) {
		//VideoFrame outputTransformed = cachedOutput;
		// Different way of waiting for a swapchain index to finish? 
		// Or would it be just setting the output to be a cached frame? 
		if (bindings[swapchaindex].activetask == nullptr
			|| bindings[swapchaindex].activetask.Status() != Windows::Foundation::AsyncStatus::Started)
		{
			OutputDebugString(L"PF Start new Eval ");
			std::wostringstream ss;
			ss << swapchaindex;
			auto idx = ss.str().c_str();
			OutputDebugString(idx);
			OutputDebugString(L" | ");

			// bind the input and the output buffers by name
			bindings[swapchaindex].binding.Bind(InputImageDescription, input);
			// submit an eval and wait for it to finish submitting work
			std::lock_guard<mutex> guard{ Processing }; // Is this still happening inside of Complete? 
			bindings[swapchaindex].activetask = Session.EvaluateAsync(bindings[swapchaindex].binding, ss.str().c_str());
			bindings[swapchaindex].activetask.Completed([&](auto&& asyncInfo, winrt::Windows::Foundation::AsyncStatus const args) {
				OutputDebugString(L"PF Eval completed | ");
				VideoFrame evalOutput = asyncInfo.GetResults().Outputs().Lookup(OutputImageDescription).try_as<VideoFrame>();
				// second lock to protect shared resource of cachedOutputCopy ? 
				{
					std::lock_guard<mutex> guard{ Copy };
					OutputDebugString(L"PF Copy | ");
					evalOutput.CopyToAsync(cachedOutputCopy).get();
				}
				cachedOutput = cachedOutputCopy;

				OutputDebugString(L"PF End ");
				//OutputDebugString(ss.str().c_str());
				//OutputDebugString(L"\n");
				});
		}
		if (cachedOutput != nullptr) {
			std::lock_guard<mutex> guard{ Copy };
			OutputDebugString(L"\nStart CopyAsync | ");
			cachedOutput.CopyToAsync(output).get();
			OutputDebugString(L"Stop CopyAsync\n");
		}
		// return without waiting for the submit to finish, setup the completion handler
	}

	void StyleTransferEffect::ProcessFrame(ProcessVideoFrameContext context) {
		OutputDebugString(L"PF Start | ");
		//OutputDebugString(index(thread().get_id()).c_str());
		auto now = std::chrono::high_resolution_clock::now();
		VideoFrame inputFrame = context.InputFrame();
		VideoFrame outputFrame = context.OutputFrame();

		SubmitEval(swapChainIndex, inputFrame, outputFrame);

		swapChainIndex = (++swapChainIndex) % swapChainEntryCount; // move on to the next entry after each call to PF. 
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

		// Create set of bindings to cycle through
		for (int i = 0; i < swapChainEntryCount; i++) {
			bindings[i].binding = LearningModelBinding(Session);
			bindings[i].binding.Bind(OutputImageDescription, VideoFrame(Windows::Graphics::Imaging::BitmapPixelFormat::Bgra8, 720, 720));
		}
	}
}
