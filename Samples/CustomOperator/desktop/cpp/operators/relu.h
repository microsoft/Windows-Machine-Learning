#pragma once

#include <MLOperatorAuthor.h>

class ReluOperator
{
public:
    struct ReluShapeInferrer : winrt::implements<ReluShapeInferrer, IMLOperatorShapeInferrer>
    {
        STDMETHOD(InferOutputShapes)(IMLOperatorShapeInferenceContext* context) noexcept
        {
            try
            {
                uint32_t inputDimsSize;
                winrt::check_hresult(context->GetInputTensorDimensionCount(0, &inputDimsSize));
        
                std::vector<uint32_t> inputDims(inputDimsSize);
                winrt::check_hresult(context->GetInputTensorShape(0, inputDimsSize, inputDims.data()));
        
                winrt::check_hresult(context->SetOutputTensorShape(0, inputDimsSize, inputDims.data()));
            }
            catch (...)
            {
                return winrt::to_hresult();
            }

            return S_OK;
        }
    };

    static MLOperatorEdgeDescription CreateEdgeDescriptor(
        MLOperatorEdgeType /*type*/,
        MLOperatorTensorDataType dataType)
    {
        MLOperatorEdgeDescription desc;
        desc.edgeType = MLOperatorEdgeType::Tensor;
        desc.tensorDataType = dataType;
        return desc;
    }

    static void RegisterReluSchema(winrt::com_ptr<IMLOperatorRegistry> registry)
    {
        MLOperatorSetId operatorSetId;
        operatorSetId.domain = "MyReluDomain";
        operatorSetId.version = 1;

        MLOperatorSchemaDescription ReluSchema = {};
        ReluSchema.name = "Relu";
        ReluSchema.operatorSetVersionAtLastChange = 1;

        MLOperatorSchemaEdgeDescription ReluXInput;
        ReluXInput.options = MLOperatorParameterOptions::Single;
        ReluXInput.typeFormat = MLOperatorSchemaEdgeTypeFormat::Label;
        ReluXInput.typeLabel = "T";

        std::vector<MLOperatorSchemaEdgeDescription> inputs { ReluXInput };
        ReluSchema.inputs = inputs.data();
        ReluSchema.inputCount = static_cast<uint32_t>(inputs.size());

        MLOperatorSchemaEdgeDescription ReluXOutput;
        ReluXOutput.options = MLOperatorParameterOptions::Single;
        ReluXOutput.typeFormat = MLOperatorSchemaEdgeTypeFormat::Label;
        ReluXOutput.typeLabel = "T";

        std::vector<MLOperatorSchemaEdgeDescription> outputs{ ReluXOutput };
        ReluSchema.outputs = outputs.data();
        ReluSchema.outputCount = static_cast<uint32_t>(outputs.size());

        MLOperatorEdgeTypeConstrant typeConstraint;
        typeConstraint.typeLabel = "T";
        std::vector<MLOperatorEdgeDescription> allowedEdges
        {
            CreateEdgeDescriptor(MLOperatorEdgeType::Tensor, MLOperatorTensorDataType::Double),
            CreateEdgeDescriptor(MLOperatorEdgeType::Tensor, MLOperatorTensorDataType::Float),
            CreateEdgeDescriptor(MLOperatorEdgeType::Tensor, MLOperatorTensorDataType::Float16)
        };
        typeConstraint.allowedTypes = allowedEdges.data();
        typeConstraint.allowedTypeCount = static_cast<uint32_t>(allowedEdges.size());

        std::vector<MLOperatorEdgeTypeConstrant> typeConstraints { typeConstraint };
        ReluSchema.typeConstraints = typeConstraints.data();
        ReluSchema.typeConstraintCount = static_cast<uint32_t>(typeConstraints.size());

        auto shareInferrer = winrt::make<ReluOperator::ReluShapeInferrer>();

        std::vector<const MLOperatorSchemaDescription*> schemas { &ReluSchema };
        winrt::check_hresult(registry->RegisterOperatorSetSchema(
            &operatorSetId,
            1 /* baseline version */,
            schemas.data(),
            static_cast<uint32_t>(schemas.size()),
            nullptr,
            shareInferrer.get()
        ));
    }
};