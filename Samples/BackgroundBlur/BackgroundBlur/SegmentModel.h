#pragma once
#include <winrt/Microsoft.AI.MachineLearning.Experimental.h>
#include <winrt/Microsoft.AI.MachineLearning.h>
#include <Windows.AI.MachineLearning.native.h>
#include <winrt/Windows.Graphics.DirectX.Direct3D11.h>
#include <windows.media.core.interop.h>
#include <strsafe.h>
#include <wtypes.h>
#include <winrt/base.h>
#include <dxgi.h>
#include <d3d11.h>
#include <mutex>
#include <winrt/windows.foundation.collections.h>
#include <winrt/Windows.Media.h>


using namespace winrt::Microsoft::AI::MachineLearning;
using namespace winrt::Microsoft::AI::MachineLearning::Experimental;
using namespace winrt::Windows::Graphics::DirectX::Direct3D11;
using namespace winrt::Windows::Media;

// Threading fields for style transfer
struct SwapChainEntry {
	LearningModelBinding bind_pre; 
	LearningModelBinding binding_model; 
	LearningModelBinding binding_post; 
	winrt::Windows::Foundation::IAsyncOperation<LearningModelEvaluationResult> activetask;
	VideoFrame outputCache;
	SwapChainEntry() :
		bind_pre(nullptr),
		binding_model(nullptr),
		binding_post(nullptr),
		activetask(nullptr),
		outputCache(NULL) {}
};


class SegmentModel {
public:
	LearningModelSession m_sess; 
	SegmentModel();
	SegmentModel(UINT32 w, UINT32 h);
	void SetModels(UINT32 w, UINT32 h);

	void Run(IDirect3DSurface src, IDirect3DSurface dest);
	void RunTestDXGI(IDirect3DSurface src, IDirect3DSurface dest);
	void RunStyleTransfer(IDirect3DSurface src, IDirect3DSurface dest);

	LearningModelSession CreateLearningModelSession(const LearningModel& model, bool closedModel=true);
	void SetImageSize(UINT32 w, UINT32 h);
	bool m_useGPU = true;
	std::mutex Processing;


private: 
	// Stages of image blurring
	LearningModel Normalize0_1ThenZScore(long height, long width, long channels, const std::array<float, 3>& means, const std::array<float, 3>& stddev);
	LearningModel FCNResnet();
	LearningModel PostProcess(long n, long c, long h, long w, long axis);

	// Debugging models 
	LearningModel ReshapeFlatBufferToNCHW(long n, long c, long h, long w);
	LearningModel Invert(long n, long c, long h, long w);
	LearningModel StyleTransfer();

	LearningModelBinding Evaluate(LearningModelSession& sess, const std::vector<ITensor*>& input, ITensor* output, bool wait = false);
	void EvaluateInternal(LearningModelSession sess, LearningModelBinding bind, bool wait = false);

	UINT32                      m_imageWidthInPixels;
	UINT32                      m_imageHeightInPixels;

	// Intermediate sessions need to be fully condensed later
	LearningModelSession m_sessPreprocess;
	LearningModelSession m_sessFCN;
	LearningModelSession m_sessPostprocess;
	LearningModelSession m_sessStyleTransfer;

	LearningModelBinding m_bindPreprocess;
	LearningModelBinding m_bindFCN;
	LearningModelBinding m_bindPostprocess;
	LearningModelBinding m_bindStyleTransfer;

	// Threaded style transfer fields
	void SubmitEval(VideoFrame, VideoFrame);
	winrt::Windows::Foundation::IAsyncOperation<LearningModelEvaluationResult> evalStatus;
	std::vector <std::unique_ptr<SwapChainEntry>> bindings;
	int swapChainIndex = 0;
	int swapChainEntryCount = 5;
	int finishedFrameIndex = 0;

};

class IStreamModel
{
public:
	IStreamModel(): 
		m_inputVideoFrame(NULL),
		m_outputVideoFrame(NULL),
		m_session(NULL),
		m_binding(NULL)
	{}
	IStreamModel(int w, int h) :
		m_inputVideoFrame(NULL),
		m_outputVideoFrame(NULL),
		m_session(NULL),
		m_binding(NULL)
	{}
	~IStreamModel() {
		if(m_session) m_session.Close();
		if(m_binding) m_binding.Clear();
	};
	virtual void SetModels(int w, int h) =0;
	virtual void Run(IDirect3DSurface src, IDirect3DSurface dest) =0;

	void SetUseGPU(bool use) { 
		m_bUseGPU = use;
	}
	void SetDevice() {
		m_device = m_session.Device().Direct3D11Device();
	}

protected:
	//virtual winrt::Windows::Foundation::IAsyncOperation<LearningModelEvaluationResult> BindInputs(VideoFrame input) = 0;

	void SetVideoFrames(VideoFrame inVideoFrame, VideoFrame outVideoFrame) 
	{
		if (!m_bVideoFramesSet)
		{
			if (m_device == NULL)
			{
				SetDevice();
			}
			auto inDesc = inVideoFrame.Direct3DSurface().Description();
			auto outDesc = outVideoFrame.Direct3DSurface().Description();
			// TODO: field width/heigh instead? 
			// TODO: Set width/height for style transfer manually
			m_inputVideoFrame = VideoFrame::CreateAsDirect3D11SurfaceBacked(inDesc.Format, 720, 720, m_device);
			m_outputVideoFrame = VideoFrame::CreateAsDirect3D11SurfaceBacked(outDesc.Format, 720, 720, m_device);
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
		auto options = LearningModelSessionOptions();
		options.BatchSizeOverride(0);
		options.CloseModelOnSessionCreation(closedModel);
		auto session = LearningModelSession(model, device);
		return session;
	}


	//// Threaded style transfer fields
	//void SubmitEval(VideoFrame, VideoFrame);
	//winrt::Windows::Foundation::IAsyncOperation<LearningModelEvaluationResult> evalStatus;
	//std::vector <std::unique_ptr<SwapChainEntry>> bindings;
	//int swapChainIndex = 0;
	//int swapChainEntryCount = 5;
	//int finishedFrameIndex = 0;

	bool						m_bUseGPU = true;
	bool						m_bVideoFramesSet = false;
	VideoFrame					m_inputVideoFrame,
								m_outputVideoFrame;
	UINT32                      m_imageWidthInPixels;
	UINT32                      m_imageHeightInPixels;
	IDirect3DDevice				m_device;

	// Learning Model Binding and Session. 
	// Derived classes can add more as needed for chaining? 
	LearningModelSession m_session;
	LearningModelBinding m_binding;

}; 

class StyleTransfer : public IStreamModel {
public:
	StyleTransfer(int w, int h) : IStreamModel(w, h) {
		SetModels(w, h); }
	void SetModels(int w, int h);
	void Run(IDirect3DSurface src, IDirect3DSurface dest);
private: 
	LearningModel GetModel();
};
