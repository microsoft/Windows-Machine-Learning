#pragma once

#include <MLOperatorAuthor.h>


struct CpuReluOperator: winrt::implements<CpuReluOperator, IMLOperatorKernel>
{
    CpuReluOperator() {}

    // Computes the outputs of the kernel.  In this case, the output will represent
    // the Rectified Linear Unit (Relu) output.
    //
    // Based on the operators location in the model graph this operator may be called multiple times
    // or simultaneously within the same instance of the class during evaluation.  Implementations
    // of this method must be thread-safe.
    STDMETHOD(Compute)(IMLOperatorKernelContext* context)
    {
        try
        {
            // Get the input tensor
            winrt::com_ptr<IMLOperatorTensor> inputTensor;
            winrt::check_hresult(context->GetInputTensor(0, inputTensor.put()));

            // Get the output tensor
            winrt::com_ptr<IMLOperatorTensor> outputTensor;
            winrt::check_hresult(context->GetOutputTensor(0, outputTensor.put()));

            // Get the input and output shape sizes
            uint32_t inputDimsSize = inputTensor->GetDimensionCount();
            uint32_t outputDimsSize = outputTensor->GetDimensionCount();
            if (inputDimsSize != outputDimsSize)
            {
                return E_UNEXPECTED;
            }

            // Get the input shape
            std::vector<uint32_t> inputDims(inputDimsSize);
            winrt::check_hresult(outputTensor->GetShape(inputDimsSize, inputDims.data()));

            // Get the output shape
            std::vector<uint32_t> outputDims(outputDimsSize);
            winrt::check_hresult(outputTensor->GetShape(outputDimsSize, outputDims.data()));

            // For the number of total elements in the input and output shapes
            auto outputDataSize = std::accumulate(outputDims.begin(), outputDims.end(), 1, std::multiplies<uint32_t>());
            auto inputDataSize = std::accumulate(inputDims.begin(), inputDims.end(), 1, std::multiplies<uint32_t>());
            if (outputDataSize != inputDataSize)
            {
                return E_UNEXPECTED;
            }

            // If the tensor types are both float type
            if (outputTensor->GetTensorDataType() == MLOperatorTensorDataType::Float &&
                inputTensor->GetTensorDataType() == MLOperatorTensorDataType::Float)
            {
                // For cpu data
                if (outputTensor->IsCpuData() && inputTensor->IsCpuData())
                {
                    ComputeInternal<float>(inputTensor.get(), outputTensor.get(), inputDataSize);
                }
            }
            else if (outputTensor->GetTensorDataType() == MLOperatorTensorDataType::Double &&
                     inputTensor->GetTensorDataType() == MLOperatorTensorDataType::Double)
            {
                // For cpu data
                if (outputTensor->IsCpuData() && inputTensor->IsCpuData())
                {
                    ComputeInternal<double>(inputTensor.get(), outputTensor.get(), inputDataSize);
                }
            }
        }
        catch (...)
        {
            return winrt::to_hresult();
        }

        return S_OK;
    }

    template <typename T, typename U = T>
    void ComputeInternal(IMLOperatorTensor* pInputTensor, IMLOperatorTensor* pOutputTensor, uint32_t size)
    {
        auto inputData = static_cast<T*>(pInputTensor->GetData());
        auto outputData = static_cast<U*>(pOutputTensor->GetData());

        for (uint32_t i = 0; i < size; i++)
        {
            outputData[i] = static_cast<U>(std::max<T>(0, inputData[i]));
        }
    }
};

struct CpuReluOperatorFactory : winrt::implements<CpuReluOperatorFactory, IMLOperatorKernelFactory>
{
    STDMETHOD(CreateKernel)(
        IMLOperatorKernelCreationContext* /*context*/,
        IMLOperatorKernel** kernel)
    {
        try
        {
            auto reluOperator = winrt::make<CpuReluOperator>();
            reluOperator.copy_to(kernel);
        }
        catch (...)
        {
            return winrt::to_hresult();
        }

        return S_OK;
    }

    static MLOperatorEdgeDescription CreateEdgeDescriptor(
        MLOperatorEdgeType /*type*/,
        MLOperatorTensorDataType dataType)
    {
        MLOperatorEdgeDescription desc;
        desc.edgeType = MLOperatorEdgeType::Tensor;
        desc.tensorDataType = dataType;
        return desc;
    }

    static void RegisterReluKernel(winrt::com_ptr<IMLOperatorRegistry> registry)
    {
        MLOperatorKernelDescription kernelDescription;
        kernelDescription.domain = "MyReluDomain";
        kernelDescription.name = "Relu";
        kernelDescription.minimumOperatorSetVersion = 1;
        kernelDescription.executionType = MLOperatorExecutionType::Cpu;

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

        std::vector<MLOperatorEdgeTypeConstrant> typeConstraints{ typeConstraint };
        kernelDescription.typeConstraints = typeConstraints.data();
        kernelDescription.typeConstraintCount = static_cast<uint32_t>(typeConstraints.size());

        kernelDescription.defaultAttributes = nullptr;
        kernelDescription.defaultAttributeCount = 0;
        kernelDescription.options = MLOperatorKernelOptions::None;
        kernelDescription.executionOptions = 0;

        auto factory = winrt::make<CpuReluOperatorFactory>();
        auto shareInferrer = winrt::make<ReluOperator::ReluShapeInferrer>();

        winrt::check_hresult(registry->RegisterOperatorKernel(
            &kernelDescription,
            factory.get(),
            shareInferrer.get()
        ));
    }
};
