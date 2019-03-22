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
		auto model = LearningModel::LoadFromFilePath(modelPath, provider);
		LearningModelSession session(model, LearningModelDevice(kind));
		LearningModelBinding binding(session);

		auto&& description = model.InputFeatures().GetAt(0);
		if (inputPath.find(L".png", inputPath.length() - 4) != std::string::npos || inputPath.find(L".jpg", inputPath.length() - 4) != std::string::npos)
		{
			// bind as image
			auto imageFeature = BindingUtilities::CreateBindableImage(description, inputPath, ImageDataType::ImageRGB);
			binding.Bind(description.Name(), imageFeature);
		}
		else if (inputPath.find(L".csv", inputPath.length() - 4) != std::string::npos) {
			// bind as tensor
			auto tensorFeature = BindingUtilities::CreateBindableTensor(description, inputPath);
			binding.Bind(description.Name(), tensorFeature);
		}

		LearningModelEvaluationResult result = session.Evaluate(binding, L"");
		return result.Succeeded();
	}

	static bool Run(std::wstring modelPath, std::wstring inputPath) 
	{
		return Run(modelPath, inputPath, LearningModelDeviceKind::Default);
	}

};


int wmain(int argc, wchar_t *argv[])

{
	if (argc < 3) {
		printf("Usage: DebugRunner.exe [model_path] [data_path]");
		return 1;
	}
	return DebugRunner::Run(std::wstring(argv[1]), std::wstring(argv[2]));
}

