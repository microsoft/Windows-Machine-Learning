#pragma once

#include "Common.h"
#include "Windows.AI.Machinelearning.Native.h"
#include "MLOperatorAuthor.h"

struct DebugShapeInferrer : winrt::implements<DebugShapeInferrer, IMLOperatorShapeInferrer>
{
	STDMETHOD(InferOutputShapes)(IMLOperatorShapeInferenceContext* context) noexcept;
};

struct DebugOperator : winrt::implements<DebugOperator, IMLOperatorKernel>
{
	winrt::hstring m_filePath;
	winrt::hstring m_fileType;

	DebugOperator(winrt::hstring filePath, winrt::hstring fileType) :
		m_filePath(filePath),
		m_fileType(fileType)
	{}

	DebugOperator(const DebugOperator &obj) :
		m_filePath(obj.m_filePath),
		m_fileType(obj.m_fileType)
	{}

	// Computes the outputs of the kernel.  This may be called multiple times
	// simultaneously within the same instance of the class.  Implementations
	// of this method must be thread-safe.
	STDMETHOD(Compute)(IMLOperatorKernelContext* context);
};

struct DebugOperatorFactory : winrt::implements<DebugOperatorFactory, IMLOperatorKernelFactory>
{
	STDMETHOD(CreateKernel)(
		IMLOperatorKernelCreationContext* context,
		IMLOperatorKernel** kernel);

	static MLOperatorEdgeDescription CreateEdgeDescriptor(MLOperatorEdgeType type, MLOperatorTensorDataType dataType);

	static void RegisterDebugSchema(winrt::com_ptr<IMLOperatorRegistry> registry);

	static void RegisterDebugKernel(winrt::com_ptr<IMLOperatorRegistry> registry);
};