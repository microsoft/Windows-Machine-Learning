#pragma once

#include "debug_cpu.h"

struct DebugOperatorProvider :
	winrt::implements<
	DebugOperatorProvider,
	winrt::Windows::AI::MachineLearning::ILearningModelOperatorProvider,
	ILearningModelOperatorProviderNative>
{
	winrt::com_ptr<IMLOperatorRegistry> m_registry;

	DebugOperatorProvider()
	{
		MLCreateOperatorRegistry(m_registry.put());

		RegisterSchemas();
		RegisterKernels();
	}

	void RegisterSchemas()
	{
		DebugOperatorFactory::RegisterDebugSchema(m_registry);
	}

	void RegisterKernels()
	{
		// Add a new operator kernel for Debug
		DebugOperatorFactory::RegisterDebugKernel(m_registry);
	}

	STDMETHOD(GetRegistry)(IMLOperatorRegistry** ppOperatorRegistry)
	{
		m_registry.copy_to(ppOperatorRegistry);
		return S_OK;
	}
};