#pragma once

#include <MLOperatorAuthor.h>

// The shader header is produced using "fxc.exe relu_shader.hlsl -E ReluShader -T cs_5_0 -Zi /Fh"
#include "generatedShaders\relu_shader.h"

#include "d3dx12.h"

// Divides and rounds up
inline uint32_t CeilDivide(uint32_t dividend, uint32_t divisor)
{
    UINT64 temp = static_cast<UINT64>(dividend) + divisor - 1;
    return static_cast<uint32_t>(temp / divisor);
}

// Gets the next number of elements to dispatch to the GPU within a loop handling a large
// total number of tensor elements and threads.
void GetNextDispatchSize(
    uint32_t elementCount, 
    uint32_t elementsPerThread, 
    uint32_t numThreads,
    _Out_ uint32_t& dispatch,
    _Out_ uint32_t& pendingElementCount
    )
{
    // Max threads per workgroup is 2^10 (1024). Max dispatch per dimension is 2^16. Taken together, we can dispatch a maximum of  
    // 2^26 (268,435,456) threads along a single dimension. This should suffice for a majority of the workload. Therefore, even  
    // though it is possible to dispatch up to (2^16)^3 workgroups simultaneously, we stick to the simpler 1D dispatch alternative.
    assert(numThreads <= D3D12_CS_THREAD_GROUP_MAX_THREADS_PER_GROUP);

    const uint32_t maxThreadsPerDispatch = numThreads * D3D12_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION;

    const uint32_t requiredThreadCount = CeilDivide(elementCount, elementsPerThread);

    // Compute max dispatchable elements
    const uint32_t availableThreadCount = std::min(requiredThreadCount, maxThreadsPerDispatch);

    // Compute required thread group count
    uint32_t workGroupCount1D = CeilDivide(availableThreadCount, numThreads); 

    // Compute min dispatch size
    dispatch = workGroupCount1D;

    // With the dispatch size computed, compute the dispatched element count
    const uint32_t dispatchedElementCount = workGroupCount1D * numThreads * elementsPerThread;

    // Update the pending element count 
    pendingElementCount = (dispatchedElementCount < elementCount) ? elementCount - dispatchedElementCount : 0;
}

struct GpuReluShapeInferrer : winrt::implements<GpuReluShapeInferrer, IMLOperatorShapeInferrer>
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
            return S_OK;
        }
        catch (...)
        {
            return winrt::to_hresult();
        }
    }
};

class GpuReluOperator: public winrt::implements<GpuReluOperator, IMLOperatorKernel>
{
public:
    GpuReluOperator(IMLOperatorKernelCreationContext* context)
    {
        winrt::com_ptr<IUnknown> executionObject;
        context->GetExecutionInterface(executionObject.put());
            
        winrt::com_ptr<ID3D12GraphicsCommandList> commandList;
        executionObject.as(commandList);

        winrt::check_hresult(commandList->GetDevice(IID_ID3D12Device, m_device.put_void()));

        PrepareGpuResources();
    }

    // Computes the outputs of the kernel.  This may be called multiple times
    // simultaneously within the same instance of the class.  Implementations
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
            context->GetOutputTensor(0, outputTensor.put());

            // Get the input and output shape sizes
            uint32_t inputDimsSize = inputTensor->GetDimensionCount();
            uint32_t outputDimsSize = outputTensor->GetDimensionCount();
            if (inputDimsSize != outputDimsSize) 
            {
                winrt::throw_hresult(E_UNEXPECTED);
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
            if (outputDataSize != inputDataSize) __fastfail(0);

            if (outputTensor->IsCpuData() || inputTensor->IsCpuData())
            {
                winrt::throw_hresult(E_UNEXPECTED);
            }
            
            if (outputTensor->GetTensorDataType() != MLOperatorTensorDataType::Float ||
                inputTensor->GetTensorDataType() != MLOperatorTensorDataType::Float)
            {
                winrt::throw_hresult(E_UNEXPECTED);
            }

            winrt::com_ptr<IUnknown> executionObject;
            winrt::com_ptr<ID3D12GraphicsCommandList> commandList;
            context->GetExecutionInterface(executionObject.put());
            executionObject.as(commandList);
            
            winrt::com_ptr<IUnknown> inputUnknown;
            winrt::com_ptr<ID3D12Resource> inputResource;
            inputTensor->GetDataInterface(inputUnknown.put());
            inputUnknown.as(inputResource);

            winrt::com_ptr<IUnknown> outputUnknown;
            winrt::com_ptr<ID3D12Resource> outputResource;
            outputTensor->GetDataInterface(outputUnknown.put());
            outputUnknown.as(outputResource);

            ComputeUsingGpu(inputResource.get(), outputResource.get(), inputDataSize, commandList.get());

            return S_OK;
        }
        catch (...)
        {
            return winrt::to_hresult();
        }
    }

    void PrepareGpuResources()
    {
        // Compute root signature.
        const int uavCount = 2;
        std::vector<CD3DX12_ROOT_PARAMETER1> rootParameters;
        rootParameters.resize(uavCount + 1);

        for (UINT i = 0; i < uavCount; i++)
        {
            rootParameters[i].InitAsUnorderedAccessView(i);
        }

        int constantCount = 2;
        rootParameters[uavCount].InitAsConstants(constantCount, 0);
    
        CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC desc;
        desc.Init_1_1(rootParameters.size(), rootParameters.data());
    
        winrt::com_ptr<ID3DBlob> rootSignatureBlob;
        winrt::com_ptr<ID3DBlob> rootSignatureErrorBlob;
        winrt::check_hresult(D3D12SerializeVersionedRootSignature(
            &desc,
            rootSignatureBlob.put(),
            rootSignatureErrorBlob.put()
            ));
    
        winrt::check_hresult(m_device->CreateRootSignature(
            0,
            rootSignatureBlob->GetBufferPointer(),
            rootSignatureBlob->GetBufferSize(),
            IID_ID3D12RootSignature,
            m_rootSignature.put_void()
            ));

        // Describe and create the compute pipeline state object (PSO).
        D3D12_COMPUTE_PIPELINE_STATE_DESC computePsoDesc = {};
        computePsoDesc.pRootSignature = m_rootSignature.get();
        computePsoDesc.CS = CD3DX12_SHADER_BYTECODE(g_Relu, sizeof(g_Relu));

        winrt::check_hresult(m_device->CreateComputePipelineState(&computePsoDesc, IID_ID3D12PipelineState, m_pipelineState.put_void()));
    }

    void ComputeUsingGpu(
        ID3D12Resource* inputResource,
        ID3D12Resource* outputResource,
        uint32_t elementCount,
        ID3D12GraphicsCommandList* commandList)
    {
        // Set the root signature and pipeline state
        commandList->SetComputeRootSignature(m_rootSignature.get());
        commandList->SetPipelineState(m_pipelineState.get());
        
        // Set resource views
        commandList->SetComputeRootUnorderedAccessView(
            0, // root parameter index
            inputResource->GetGPUVirtualAddress()
            );

        commandList->SetComputeRootUnorderedAccessView(
            1, // root parameter index
            outputResource->GetGPUVirtualAddress()
            );

        // Transition resources from common to UAV state
        D3D12_RESOURCE_BARRIER barriers[2];

        barriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(
                inputResource,
                D3D12_RESOURCE_STATE_COMMON,
                D3D12_RESOURCE_STATE_UNORDERED_ACCESS
                );

        barriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(
                outputResource,
                D3D12_RESOURCE_STATE_COMMON,
                D3D12_RESOURCE_STATE_UNORDERED_ACCESS
                );
        
        commandList->ResourceBarrier(2, barriers);

        // Dispatch in a loop
        struct
        {
            uint32_t startIndex;
            uint32_t elementCount;
        } constants;

        constants.elementCount = elementCount;
        uint32_t pendingElementCount  = elementCount;
    
        // Dispatch up to the maximum number of threads per iteration until
        // all elements are completed
        while (pendingElementCount > 0)
        {
            constants.startIndex = elementCount - pendingElementCount;

            uint32_t dispatchSizeX;
        
            GetNextDispatchSize(
                pendingElementCount, 
                1,
                64, 
                dispatchSizeX, 
                pendingElementCount
                );
            
            // Set root constants
            commandList->SetComputeRoot32BitConstants(
                2, // root parameter index
                2, // Constant count
                &constants, 
                0 // offset
                );

            commandList->Dispatch(dispatchSizeX, 1, 1);
        }    

        // Transition resources to common state
        barriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(
                inputResource,
                D3D12_RESOURCE_STATE_UNORDERED_ACCESS, 
                D3D12_RESOURCE_STATE_COMMON
                );

        barriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(
                outputResource,
                D3D12_RESOURCE_STATE_UNORDERED_ACCESS, 
                D3D12_RESOURCE_STATE_COMMON
                );
        
        commandList->ResourceBarrier(2, barriers);
    }


private:
    winrt::com_ptr<ID3D12Device> m_device;
    winrt::com_ptr<ID3D12RootSignature> m_rootSignature;
    winrt::com_ptr<ID3D12PipelineState> m_pipelineState;
};

class GpuReluOperatorFactory : public winrt::implements<GpuReluOperatorFactory, IMLOperatorKernelFactory>
{
public:
    STDMETHOD(CreateKernel)(
        IMLOperatorKernelCreationContext* context,
        IMLOperatorKernel** kernel)
    {
        try
        {
            auto reluOperator = winrt::make<GpuReluOperator>(context);
            reluOperator.copy_to(kernel);
            return S_OK;
        }
        catch (...)
        {
            return winrt::to_hresult();
        }
    }

    static void RegisterReluKernel(winrt::com_ptr<IMLOperatorRegistry> registry)
    {
        MLOperatorKernelDescription kernelDescription = {};
        kernelDescription.domain = "MyReluDomain";
        kernelDescription.name = "Relu";
        kernelDescription.minimumOperatorSetVersion = 1;
        kernelDescription.executionType = MLOperatorExecutionType::D3D12;

        MLOperatorEdgeTypeConstrant typeConstraint;
        typeConstraint.typeLabel = "T";
        std::vector<MLOperatorEdgeDescription> allowedEdges
        {
            ReluOperator::CreateEdgeDescriptor(MLOperatorEdgeType::Tensor, MLOperatorTensorDataType::Float),
        };
        typeConstraint.allowedTypes = allowedEdges.data();
        typeConstraint.allowedTypeCount = static_cast<uint32_t>(allowedEdges.size());

        std::vector<MLOperatorEdgeTypeConstrant> typeConstraints{ typeConstraint };
        kernelDescription.typeConstraints = typeConstraints.data();
        kernelDescription.typeConstraintCount = static_cast<uint32_t>(typeConstraints.size());
        kernelDescription.options = MLOperatorKernelOptions::None;
        kernelDescription.executionOptions = 0;
        
        auto shareInferrer = winrt::make<ReluOperator::ReluShapeInferrer>();
        auto factory = winrt::make<GpuReluOperatorFactory>();

        winrt::check_hresult(registry->RegisterOperatorKernel(
            &kernelDescription,
            factory.get(),
            shareInferrer.get()
        ));

    }
};
