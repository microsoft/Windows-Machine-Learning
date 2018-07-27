#pragma once
#include "Common.h"
#include "ModelBinding.h"

using namespace winrt::Windows::AI::MachineLearning;
namespace BindingUtilities
{

    void BindTensorsFromGarbageData(LearningModelBinding context, LearningModel model) {
        for (auto&& description : model.InputFeatures())
        {
            if (description == nullptr)
            {
                ThrowFailure(L" Learning model has no binding description.");
            }

            hstring name = description.Name();
            TensorFeatureDescriptor tensorDescriptor = description.as<TensorFeatureDescriptor>();
            TensorKind tensorKind = tensorDescriptor.TensorKind();

            switch (tensorKind) {
            case TensorKind::Undefined:
            {
                ThrowFailure(L" TensorKind is undefined.");
            }
            case TensorKind::Float:
            {
                ModelBinding<float> binding(description);
                ITensor tensor = TensorFloat::CreateFromArray(binding.GetShapeBuffer(), binding.GetDataBuffer());
                context.Bind(name, tensor);
            }
                break;
            case TensorKind::Double:
            {
                ModelBinding<double> binding(description);
                ITensor tensor = TensorDouble::CreateFromArray(binding.GetShapeBuffer(), binding.GetDataBuffer());
                context.Bind(name, tensor);
            }
                break;
            case TensorKind::Int8:
            {
                ModelBinding<uint8_t> binding(description);
                ITensor tensor = TensorInt8Bit::CreateFromArray(binding.GetShapeBuffer(), binding.GetDataBuffer());
                context.Bind(name, tensor);
            }
                break;
            case TensorKind::UInt8:
            {
                ModelBinding<uint8_t> binding(description);
                ITensor tensor = TensorUInt8Bit::CreateFromArray(binding.GetShapeBuffer(), binding.GetDataBuffer());
                context.Bind(name, tensor);
            }
                break;
            case TensorKind::Int16:
            {
                ModelBinding<int16_t> binding(description);
                ITensor tensor = TensorInt16Bit::CreateFromArray(binding.GetShapeBuffer(), binding.GetDataBuffer());
                context.Bind(name, tensor);
            }
                break;
            case TensorKind::UInt16:
            {
                ModelBinding<uint16_t> binding(description);
                ITensor tensor = TensorUInt16Bit::CreateFromArray(binding.GetShapeBuffer(), binding.GetDataBuffer());
                context.Bind(name, tensor);
            }
                break;
            case TensorKind::Int32:
            {
                ModelBinding<int32_t> binding(description);
                ITensor tensor = TensorInt32Bit::CreateFromArray(binding.GetShapeBuffer(), binding.GetDataBuffer());
                context.Bind(name, tensor);
            }
                break;
            case TensorKind::UInt32:
            {
                ModelBinding<uint32_t> binding(description);
                ITensor tensor = TensorUInt32Bit::CreateFromArray(binding.GetShapeBuffer(), binding.GetDataBuffer());
                context.Bind(name, tensor);
            }
                break;
            case TensorKind::Int64:
            {
                ModelBinding<int64_t> binding(description);
                ITensor tensor = TensorInt64Bit::CreateFromArray(binding.GetShapeBuffer(), binding.GetDataBuffer());
                context.Bind(name, tensor);
            }
                break;
            case TensorKind::UInt64:
            {
                ModelBinding<uint64_t> binding(description);
                ITensor tensor = TensorUInt64Bit::CreateFromArray(binding.GetShapeBuffer(), binding.GetDataBuffer());
                context.Bind(name, tensor);
            }
                break;
            default:
            {
                ThrowFailure(L"TensorKind has not been implemented.");
                break;
            }
            }
        }
    }

    void BindGarbageDataToContext(LearningModelBinding context, LearningModel model) {
        context.Clear();
         BindTensorsFromGarbageData(context, model);
    }
};
