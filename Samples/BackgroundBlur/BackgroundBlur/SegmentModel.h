#pragma once
#include <winrt/Microsoft.AI.MachineLearning.Experimental.h>
#include <winrt/Microsoft.AI.MachineLearning.h>
#include <Windows.AI.MachineLearning.native.h>
#include <strsafe.h>
#include <wtypes.h>
#include <winrt/base.h>

using namespace winrt::Microsoft::AI::MachineLearning;
using namespace winrt::Microsoft::AI::MachineLearning::Experimental;

class SegmentModel {
public:
	LearningModelSession m_sess; 
	SegmentModel();
	SegmentModel(UINT32 w, UINT32 h);

	void Run(const BYTE** pSrc, BYTE** pDest, DWORD cbImageSize);
	void RunTest(const BYTE** pSrc, BYTE** pDest, DWORD cbImageSize);

	LearningModelSession CreateLearningModelSession(LearningModel model, bool closedModel=true);
	void SetImageSize(UINT32 w, UINT32 h);

private: 
	LearningModel GetBackground(long n, long c, long h, long w);
	LearningModel GetForeground(long n, long c, long h, long w);
	LearningModel Argmax(long axis, long h, long w);
	LearningModel FCNResnet();
	LearningModel ReshapeFlatBufferToNCHW(long n, long c, long h, long w);
	LearningModel ReshapeFlatBufferToNCHWAndInvert(long n, long c, long h, long w);
	LearningModel Normalize0_1ThenZScore(long height, long width, long channels, std::vector<float> means, std::vector<float> stddev);

	LearningModelBinding Evaluate(LearningModelSession sess, std::vector<ITensor*> input, ITensor* output, bool wait = false);

	UINT32                      m_imageWidthInPixels;
	UINT32                      m_imageHeightInPixels;

	// Intermediate sessions need to be condensed later
	LearningModelSession m_bufferSess;
};