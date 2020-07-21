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
	struct StyleTransferEffect : StyleTransferEffectT<StyleTransferEffect>
	{
		StyleTransferEffect();
		VideoFrame outputTransformed;
		VideoFrame inputTransformed;

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

	private:
		LearningModelSession Session;
		LearningModelBinding Binding;
		hstring InputImageDescription;
		hstring OutputImageDescription;
		VideoEncodingProperties encodingProperties;
		std::mutex Processing;
		StyleTransferEffectNotifier Notifier;
		std::chrono::time_point<std::chrono::steady_clock> m_StartTime;
		bool firstProcessFrameCall = true;
		Windows::Graphics::Imaging::BitmapBounds copyBounds;
	};
}

namespace winrt::StyleTransferEffectCpp::factory_implementation
{
	struct StyleTransferEffect : StyleTransferEffectT<StyleTransferEffect, implementation::StyleTransferEffect>
	{
	};
}
