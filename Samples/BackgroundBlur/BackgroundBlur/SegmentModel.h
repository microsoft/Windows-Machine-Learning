#pragma once
#include <winrt/Microsoft.AI.MachineLearning.Experimental.h>
#include <winrt/Microsoft.AI.MachineLearning.h>
#include <strsafe.h>
#include <wtypes.h>
#include <winrt/base.h>

using namespace winrt::Microsoft::AI::MachineLearning;
using namespace winrt::Microsoft::AI::MachineLearning::Experimental;

// TODO: Implement IUnknown? 
class SegmentModel {
public:
	LearningModelSession m_sess; 
	SegmentModel();
	SegmentModel(UINT32 w, UINT32 h);

	void Run(const BYTE** pSrc, BYTE** pDest, DWORD cbImageSize);

	LearningModelSession CreateLearningModelSession(LearningModel model, bool closedModel=true);
	void SetImageSize(UINT32 w, UINT32 h);

private: 
	LearningModel ConvertToGrayScale(long n, long c, long h, long w);
	LearningModel ReshapeFlatBufferToNCHW(long n, long c, long h, long w);
	LearningModelBinding Evaluate(ITensor* input, ITensor* output, bool wait = false);

	UINT32                      m_imageWidthInPixels;
	UINT32                      m_imageHeightInPixels;
};