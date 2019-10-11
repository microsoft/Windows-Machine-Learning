#pragma once

#include "noisyrelu_cpu.h"
#include "relu.h"
#include "relu_cpu.h"
#include "relu_gpu.h"
#include "debug_cpu.h"

struct CustomOperatorProvider :
    winrt::implements<
        CustomOperatorProvider,
        winrt::Windows::AI::MachineLearning::ILearningModelOperatorProvider,
        ILearningModelOperatorProviderNative>
{
    winrt::com_ptr<IMLOperatorRegistry> m_registry;

    CustomOperatorProvider()
    {
        MLCreateOperatorRegistry(m_registry.put());

        RegisterSchemas();
        RegisterKernels();
    }

    void RegisterSchemas()
    {
        // Register a Relu operator schemain a custom domain ("MyReluDomain" version 1)
        ReluOperator::RegisterReluSchema(m_registry);
		
        // Register a NoisyRelu operator schemain a custom domain ("MyNoisyReluDomain" version 1)
        NoisyReluOperatorFactory::RegisterNoisyReluSchema(m_registry);

        DebugOperatorFactory::RegisterDebugSchema(m_registry);
    }

    void RegisterKernels()
    {
        // Register a CPU Relu kernel in a custom domain ("MyReluDomain" version 1)
        CpuReluOperatorFactory::RegisterReluKernel(m_registry);

        // Register a GPU Relu kernel in a custom domain ("MyReluDomain" version 1)
        GpuReluOperatorFactory::RegisterReluKernel(m_registry);

        // Add a new operator kernel for Relu
        NoisyReluOperatorFactory::RegisterNoisyReluKernel(m_registry);

        // Add a new operator kernel for Debug
        DebugOperatorFactory::RegisterDebugKernel(m_registry);
    }

    STDMETHOD(GetRegistry)(IMLOperatorRegistry** ppOperatorRegistry)
    {
        m_registry.copy_to(ppOperatorRegistry);
        return S_OK;
    }
};
