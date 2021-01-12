// SqueezeNetObjectDetectionCPPNonWinRT.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include "../pch.h"
#include "Microsoft.AI.MachineLearning.Native.h"
#include "abi/Microsoft.AI.MachineLearning.h"
#include "raw/microsoft.ai.machinelearning.h"
#include "raw/microsoft.ai.machinelearning.gpu.h"
#include "Filehelper.h"
namespace ml = Microsoft::AI::MachineLearning;

int main()
{
    std::wstring model_path = FileHelper::GetModulePath() + L".\\SqueezeNet.onnx";
    std::unique_ptr<ml::learning_model> model = std::make_unique<ml::learning_model>(model_path.c_str(), model_path.size());
    std::unique_ptr<ml::learning_model_device> device = std::make_unique<ml::learning_model_device>();

    const wchar_t input_name[] = L"data_0";
    const wchar_t output_name[] = L"softmaxout_1";

    std::unique_ptr<ml::learning_model_session> session = std::make_unique<ml::learning_model_session>(*model, *device);

    std::unique_ptr<ml::learning_model_binding> binding = std::make_unique<ml::learning_model_binding>(*session.get());

    auto input_shape = std::vector<ml::tensor_shape_type>{ 1, 3, 224, 224 };
    auto input_data = std::vector<float>(1 * 3 * 224 * 224);
    auto output_shape = std::vector<ml::tensor_shape_type>{ 1, 1000, 1, 1 };

    std::iota(begin(input_data), end(input_data), 0.f);

    binding->bind<float>(
        input_name, _countof(input_name) - 1,
        input_shape.data(), input_shape.size(),
        input_data.data(), input_data.size());


    ml::learning_model_results results = session->evaluate(*binding.get());
    float* p_buffer = nullptr;
    size_t buffer_size = 0;
    bool succeeded = 0 == results.get_output(
        output_name,
        _countof(output_name) - 1,
        reinterpret_cast<void**>(&p_buffer),
        &buffer_size);

}