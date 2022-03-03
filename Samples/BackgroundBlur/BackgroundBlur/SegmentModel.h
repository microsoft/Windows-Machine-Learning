#define USE_PIX
#define DBG

#include <winrt/Microsoft.AI.MachineLearning.Experimental.h>
#include <winrt/Microsoft.AI.MachineLearning.h>
#include <Windows.AI.MachineLearning.native.h>
#include <winrt/Windows.Graphics.DirectX.Direct3D11.h>
#include <windows.media.core.interop.h>
#include <winrt/Windows.Devices.Display.Core.h>
#include <strsafe.h>
#include <wtypes.h>
#include <winrt/base.h>
#include <dxgi.h>
#include <d3d11.h>
#include <mutex>
#include <winrt/windows.foundation.collections.h>
#include <winrt/Windows.Media.h>
//#include <DXProgrammableCapture.h>
#include "common.h"


using namespace winrt::Microsoft::AI::MachineLearning;
using namespace winrt::Microsoft::AI::MachineLearning::Experimental;
using namespace winrt::Windows::Graphics::DirectX::Direct3D11;
using namespace winrt::Windows::Media;



// Model-agnostic helper LearningModels
LearningModel Normalize0_1ThenZScore(long height, long width, long channels, const std::array<float, 3>& means, const std::array<float, 3>& stddev);
LearningModel ReshapeFlatBufferToNCHW(long n, long c, long h, long w);
LearningModel Invert(long n, long c, long h, long w);

//winrt::com_ptr<IDXGraphicsAnalysis> pGraphicsAnalysis;

class IStreamModel
{
public:
	IStreamModel(): 
		m_inputVideoFrame(NULL),
		m_outputVideoFrame(NULL),
		m_session(NULL),
		m_binding(NULL), 
		m_bSyncStarted(FALSE)
	{}
	IStreamModel(int w, int h) :
		m_inputVideoFrame(NULL),
		m_outputVideoFrame(NULL),
		m_session(NULL),
		m_binding(NULL),
		m_bSyncStarted(FALSE)
	{}
	~IStreamModel() {
		if(m_session) m_session.Close();
		if(m_binding) m_binding.Clear();
		if (m_inputVideoFrame) m_inputVideoFrame.Close();
		if (m_outputVideoFrame) m_outputVideoFrame.Close();
		if (m_device) m_device.Close();
	};
	virtual void SetModels(int w, int h) =0;
	virtual void Run(IDirect3DSurface src, IDirect3DSurface dest) =0;
	virtual VideoFrame RunAsync(IDirect3DSurface src, IDirect3DSurface dest) = 0;

	void SetUseGPU(bool use) { 
		m_bUseGPU = use;
	}
	void SetDevice() {
		assert(m_session.Device().AdapterId() == nvidia);
		assert(m_session.Device().Direct3D11Device() != NULL);
		m_device = m_session.Device().Direct3D11Device();
		auto device = m_session.Device().AdapterId();
	}
	winrt::Windows::Foundation::IAsyncOperation<LearningModelEvaluationResult> m_evalStatus;
	BOOL m_bSyncStarted; // TODO: Construct an IStreamModel as sync/async, then have a GetStatus to query this or m_evalStatus
	VideoFrame m_outputVideoFrame;
	std::condition_variable m_canRunEval;
	std::mutex Processing;
	std::mutex m_runMutex;

protected:
	winrt::Windows::Graphics::DisplayAdapterId nvidia{};

	void SetVideoFrames(VideoFrame inVideoFrame, VideoFrame outVideoFrame) 
	{
		if (true || !m_bVideoFramesSet)
		{
			if (m_device == NULL)
			{
				SetDevice();
			}
			auto inDesc = inVideoFrame.Direct3DSurface().Description();
			auto outDesc = outVideoFrame.Direct3DSurface().Description();
			/*
				NOTE: VideoFrame::CreateAsDirect3D11SurfaceBacked takes arguments in (width, height) order
				whereas every model created with LearningModelBuilder takes arguments in (height, width) order. 
			*/ 
			auto format = winrt::Windows::Graphics::DirectX::DirectXPixelFormat::B8G8R8X8UIntNormalized;
			m_inputVideoFrame = VideoFrame::CreateAsDirect3D11SurfaceBacked(format, m_imageWidthInPixels, m_imageHeightInPixels, m_device);
			m_outputVideoFrame = VideoFrame::CreateAsDirect3D11SurfaceBacked(format, m_imageWidthInPixels, m_imageHeightInPixels, m_device);
			m_bVideoFramesSet = true;
		}
		// TODO: Fix bug in WinML so that the surfaces from capture engine are shareable, remove copy. 
		inVideoFrame.CopyToAsync(m_inputVideoFrame).get();
		outVideoFrame.CopyToAsync(m_outputVideoFrame).get();
	}

	void SetImageSize(int w, int h) {
		m_imageWidthInPixels = w;
		m_imageHeightInPixels = h;
	}

	LearningModelSession CreateLearningModelSession(const LearningModel& model, bool closedModel = true) {
		auto device = m_bUseGPU ? LearningModelDevice(LearningModelDeviceKind::DirectXHighPerformance) : LearningModelDevice(LearningModelDeviceKind::Default);
		auto displayAdapter = winrt::Windows::Devices::Display::Core::DisplayAdapter::FromId(device.AdapterId());
		nvidia = device.AdapterId();
		auto options = LearningModelSessionOptions();
		options.BatchSizeOverride(0);
		options.CloseModelOnSessionCreation(closedModel);
		auto session = LearningModelSession(model, device, options);
		return session;
	}

	bool						m_bUseGPU = true;
	bool						m_bVideoFramesSet = false;
	VideoFrame					m_inputVideoFrame;
								
	UINT32                      m_imageWidthInPixels;
	UINT32                      m_imageHeightInPixels;
	IDirect3DDevice				m_device;

	// TODO: IAsyncOperation<LearningModelResult> that RunAsync sets, TransformAsync can query to see if this model is doing work

	// Learning Model Binding and Session. 
	// Derived classes can add more as needed for chaining? 
	LearningModelSession m_session;
	LearningModelBinding m_binding;

}; 

// TODO: Make an even more Invert IStreamModel? 
//class Invert : public IStreamModel
//{
//public: 
//	Invert(int w, int h) : IStreamModel(w, h) { SetModels(w, h); }
//	Invert() : IStreamModel() {}
//	void SetModels(int w, int h); 
//	void Run(IDirect3DSurface src, IDirect3DSurface dest);
//};

class StyleTransfer : public IStreamModel {
public:
	StyleTransfer(int w, int h) : IStreamModel(w, h) {
		SetModels(w, h); }
	StyleTransfer() : IStreamModel() {};
	~StyleTransfer(){};
	void SetModels(int w, int h);
	void Run(IDirect3DSurface src, IDirect3DSurface dest);
	VideoFrame RunAsync(IDirect3DSurface src, IDirect3DSurface dest);
private: 
	LearningModel GetModel();
};


class BackgroundBlur : public IStreamModel
{
public:
	BackgroundBlur(int w, int h) : 
		IStreamModel(w, h), 
		m_sessionPreprocess(NULL),
		m_sessionPostprocess(NULL),
		m_bindingPreprocess(NULL),
		m_bindingPostprocess(NULL), 
		m_sessionFused(NULL),
		m_bindFused(NULL)
	{
		SetModels(w, h);
	}
	BackgroundBlur() : 
		IStreamModel(),
		m_sessionPreprocess(NULL),
		m_sessionPostprocess(NULL),
		m_bindingPreprocess(NULL),
		m_bindingPostprocess(NULL),
		m_sessionFused(NULL),
		m_bindFused(NULL)
	{};
	~BackgroundBlur();
	void SetModels(int w, int h);
	void Run(IDirect3DSurface src, IDirect3DSurface dest);
	VideoFrame RunAsync(IDirect3DSurface src, IDirect3DSurface dest);

private:
	LearningModel GetModel();
	LearningModel PostProcess(long n, long c, long h, long w, long axis);

	std::mutex Processing; // Ensure only one access to a BB model at a time? 

	// Trying to used a fused learningmodelexperimental 
	LearningModelSession m_sessionFused; 
	LearningModelBinding m_bindFused;

	// Background blur-specific sessions, bindings 
	LearningModelSession m_sessionPreprocess; 
	LearningModelSession m_sessionPostprocess; 
	LearningModelBinding m_bindingPreprocess;
	LearningModelBinding m_bindingPostprocess; 
};