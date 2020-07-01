#pragma once

#include "Class.g.h"
#include "winrt\Windows.Media.Effects.h"

using namespace winrt::Windows::Media::Effects;
using namespace winrt::Windows::Media::MediaProperties;
using namespace winrt::Windows::Graphics::DirectX::Direct3D11;
using namespace winrt::Windows::Foundation::Collections;
using namespace winrt::Microsoft::AI::MachineLearning;
using namespace winrt::Windows::Media;

namespace winrt::RuntimeComponent2::implementation
{
	struct Class : ClassT<Class>
	{
		Class() = default;

		IVectorView<VideoEncodingProperties> SupportedEncodingProperties();
		bool TimeIndependent();
		MediaMemoryTypes SupportedMemoryTypes();
		bool IsReadOnly();
		IPropertySet configuration;

		void MyProperty(int32_t value);
		void DiscardQueuedFrames();
		void Close(MediaEffectClosedReason);
		void ProcessFrame(ProcessVideoFrameContext);
		void SetEncodingProperties(VideoEncodingProperties, IDirect3DDevice);
		void SetProperties(IPropertySet);

	private:
		LearningModelSession Session();
		LearningModelBinding Binding();
		hstring InputImageDescription();
		hstring OutputImageDescription();
		VideoEncodingProperties encodingProperties;
	};
}

namespace winrt::RuntimeComponent2::factory_implementation
{
	struct Class : ClassT<Class, implementation::Class>
	{
	};
}
