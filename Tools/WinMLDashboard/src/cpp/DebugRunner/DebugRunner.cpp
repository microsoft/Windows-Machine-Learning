#pragma once
#include <windows.h>
#include <winrt/Windows.AI.MachineLearning.h>
#include <Windows.AI.MachineLearning.Native.h>
#include "debugoperatorprovider.h"
#include "debug_cpu.h"
#include "BindingUtilities.h"

using namespace winrt::Windows::AI::MachineLearning;

class DebugRunner {
public:

	static bool Run(std::wstring modelPath, std::wstring inputPath, LearningModelDeviceKind kind) 
	{
		auto customOperatorProvider = winrt::make<DebugOperatorProvider>();
		auto provider = customOperatorProvider.as<ILearningModelOperatorProvider>();
		auto model = LearningModel::LoadFromFilePath(modelPath);
		LearningModelSession session(model, LearningModelDevice(kind));
		LearningModelBinding binding(session);

		auto&& description = model.InputFeatures().GetAt(0);
		if (modelPath.find(L".png") != std::string::npos || modelPath.find(L".jpg") != std::string::npos)
		{
			// bind as image
			auto imageFeature = BindingUtilities::CreateBindableImage(description, inputPath, ImageDataType::ImageRGB);
			binding.Bind(description.Name(), imageFeature);
		}
		else {
			// bind as tensor
			auto tensorFeature = BindingUtilities::CreateBindableTensor(description, inputPath);
			binding.Bind(description.Name(), tensorFeature);

		}

		LearningModelEvaluationResult result = session.Evaluate(binding, L"");
		return result.Succeeded();
	}

	static bool Run(std::wstring modelPath, std::wstring inputPath) 
	{
		Run(modelPath, inputPath, LearningModelDeviceKind::Default);
	}

};

#include "nbind/nbind.h"

NBIND_CLASS(DebugRunner) {
	method(Run);
}


