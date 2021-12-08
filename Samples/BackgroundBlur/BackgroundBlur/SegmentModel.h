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

using namespace winrt::Microsoft::AI::MachineLearning;
using namespace winrt::Microsoft::AI::MachineLearning::Experimental;
using namespace winrt::Windows::Graphics::DirectX::Direct3D11;
using namespace winrt::Windows::Media;

class SegmentModel {
public:
	LearningModelSession m_sess; 
	SegmentModel();
	SegmentModel(UINT32 w, UINT32 h);

	IDirect3DSurface Run(const IDirect3DSurface& pSrc, const DWORD cbImageSize);
	
	void RunTest(const BYTE** pSrc, BYTE** pDest, const DWORD cbImageSize);
	void RunTestDXGI(IDirect3DSurface src, IDirect3DSurface dest);

	LearningModelSession CreateLearningModelSession(const LearningModel& model, bool closedModel=true);
	void SetImageSize(UINT32 w, UINT32 h);
	bool m_useGPU = true;

private: 
	LearningModel PostProcess(long n, long c, long h, long w, long axis);
	LearningModel FCNResnet();


	LearningModel GetBackground(long n, long c, long h, long w);
	LearningModel GetForeground(long n, long c, long h, long w);
	LearningModel Argmax(long axis, long h, long w);
	LearningModel ReshapeFlatBufferToNCHW(long n, long c, long h, long w);
	LearningModel ReshapeFlatBufferToNCHWAndInvert(long n, long c, long h, long w);
	LearningModel Normalize0_1ThenZScore(long height, long width, long channels, const std::array<float, 3>& means, const std::array<float,3>& stddev);
	LearningModel Invert(long n, long c, long h, long w);

	LearningModelBinding Evaluate(LearningModelSession& sess, const std::vector<ITensor*>& input, ITensor* output, bool wait = false);
	void EvaluateInternal(LearningModelSession sess, LearningModelBinding bind, bool wait = false);

	UINT32                      m_imageWidthInPixels;
	UINT32                      m_imageHeightInPixels;

	// Intermediate sessions need to be condensed later
	// TODO: Keep as smart pointers instead
	LearningModelSession m_sessPreprocess;
	LearningModelSession m_sessFCN;
	LearningModelSession m_sessPostprocess;

	LearningModelBinding m_bindPreprocess;
	LearningModelBinding m_bindFCN;
	LearningModelBinding m_bindPostprocess;

};