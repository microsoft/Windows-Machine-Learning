#pragma once

#include "StyleTransferEffect.g.h"

using namespace winrt::Windows::Media::Effects;
using namespace winrt::Windows::Media::MediaProperties;
using namespace winrt::Windows::Graphics::DirectX::Direct3D11;
using namespace winrt::Windows::Foundation::Collections;
using namespace winrt::Microsoft::AI::MachineLearning;
using namespace winrt::Windows::Media;

namespace winrt::StyleTransferEffectCpp::implementation
{
	struct SwapChainEntry {
		LearningModelBinding binding;
		Windows::Foundation::IAsyncOperation<LearningModelEvaluationResult> activetask;
		VideoFrame transformOutput;
		SwapChainEntry() :
			binding(nullptr),
			activetask(nullptr),
			transformOutput(Windows::Graphics::Imaging::BitmapPixelFormat::Bgra8, 720, 720) {}
	};

	struct StyleTransferEffect : StyleTransferEffectT<StyleTransferEffect>
	{
		StyleTransferEffect();
		VideoFrame outputTransformed;
		IVectorView<VideoEncodingProperties> SupportedEncodingProperties();
		bool TimeIndependent();
		MediaMemoryTypes SupportedMemoryTypes();
		bool IsReadOnly();
		IPropertySet configuration;

		void DiscardQueuedFrames();
		void Close(MediaEffectClosedReason);
		void ProcessFrame(ProcessVideoFrameContext);
		void SetEncodingProperties(VideoEncodingProperties, IDirect3DDevice);
		void SetProperties(IPropertySet);
		void SubmitEval(int, VideoFrame, VideoFrame, VideoFrame);

	private:
		LearningModelSession Session;
		LearningModelBinding Binding;

		VideoEncodingProperties encodingProperties;
		std::mutex Processing;
		std::mutex Copy;

		StyleTransferEffectNotifier Notifier;
		std::chrono::time_point<std::chrono::steady_clock> m_StartTime;
		bool firstProcessFrameCall = true;
		Windows::Graphics::Imaging::BitmapBounds copyBounds;
		Windows::Foundation::IAsyncOperation<LearningModelEvaluationResult> evalStatus;
		VideoFrame cachedOutput;
		VideoFrame cachedOutputCopy;
		hstring InputImageDescription;
		hstring OutputImageDescription;
		int swapChainIndex = 0;
		static const int swapChainEntryCount = 10;
		SwapChainEntry bindings[swapChainEntryCount];

		void EvaluateComplete(VideoFrame);
	};
}

namespace winrt::StyleTransferEffectCpp::factory_implementation
{
	struct StyleTransferEffect : StyleTransferEffectT<StyleTransferEffect, implementation::StyleTransferEffect>
	{
	};
}
