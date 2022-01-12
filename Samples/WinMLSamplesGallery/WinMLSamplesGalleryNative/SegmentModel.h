#pragma once
#include <winrt/Microsoft.AI.MachineLearning.Experimental.h>
#include <winrt/Microsoft.AI.MachineLearning.h>
#include <Windows.AI.MachineLearning.native.h>
#include <windows.media.core.interop.h>
#include <strsafe.h>
#include <wtypes.h>
#include <winrt/base.h>
#include <dxgi.h>
#include <d3d11.h>
#include <mutex>

using namespace winrt::Microsoft::AI::MachineLearning;
using namespace winrt::Microsoft::AI::MachineLearning::Experimental;
using namespace winrt::Windows::Graphics::DirectX::Direct3D11;
using namespace winrt::Windows::Media;

// Threading fields for style transfer
//struct SwapChainEntry {
//	LearningModelBinding binding; // Just one for style transfer, for now
//	winrt::Windows::Foundation::IAsyncOperation<LearningModelEvaluationResult> activetask;
//	VideoFrame outputCache;
//	SwapChainEntry() :
//		binding(nullptr),
//		activetask(nullptr),
//		outputCache(VideoFrame(winrt::Windows::Graphics::Imaging::BitmapPixelFormat::Bgra8, 720, 720)) {}
//};


class SegmentModel {
public:
	LearningModelSession m_sess; 
	SegmentModel();
	SegmentModel(UINT32 w, UINT32 h);
	void SetModels(UINT32 w, UINT32 h);

	void Run(IDirect3DSurface src, IDirect3DSurface dest, IDirect3DDevice device);
	void RunTestDXGI(IDirect3DSurface src, IDirect3DSurface dest, IDirect3DDevice device);
	void RunStyleTransfer(IDirect3DSurface src, IDirect3DSurface dest, IDirect3DDevice device);

	LearningModelSession CreateLearningModelSession(const LearningModel& model, bool closedModel=true);
	void SetImageSize(UINT32 w, UINT32 h);
	bool m_useGPU = true;
	std::mutex Processing;

	// Hacky way to pass model path
	winrt::hstring modelPath = L"";

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
	//std::vector < std::unique_ptr<SwapChainEntry>> bindings;
	int swapChainIndex = 0;
	int swapChainEntryCount = 5;
	int finishedFrameIndex = 0;


};