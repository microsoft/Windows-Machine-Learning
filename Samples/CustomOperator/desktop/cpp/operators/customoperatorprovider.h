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
        ReluOperator::RegisterReluSchema(m_registry);
        NoisyReluOperatorFactory::RegisterNoisyReluSchema(m_registry);

        DebugOperatorFactory::RegisterDebugSchema(m_registry);
    }

    void RegisterKernels()
    {
        // Replace the Relu operator kernel
        CpuReluOperatorFactory::RegisterReluKernel(m_registry);

        // Replace the Relu operator kernel
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
