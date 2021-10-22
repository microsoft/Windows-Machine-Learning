#pragma once
#include <winrt/Microsoft.AI.MachineLearning.Experimental.h>
#include <winrt/Microsoft.AI.MachineLearning.h>
#include <strsafe.h>

using namespace winrt::Microsoft::AI::MachineLearning;
using namespace winrt::Microsoft::AI::MachineLearning::Experimental;

class SegmentModel {
public:
	LearningModelSession m_sess; 
	SegmentModel();

	LearningModelSession CreateLearningModelSession(LearningModel model, bool closedModel=true);
private: 
	LearningModel ConvertToGrayScale(long n, long c, long h, long w);
	LearningModel ReshapeFlatBufferToNCHW(long n, long c, long h, long w);
};