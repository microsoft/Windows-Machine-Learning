#pragma once
#include "Common.h"

// Stores data and size of input and output bindings.
template< typename T>
class ModelBinding
{
public:
    ModelBinding(winrt::Windows::AI::MachineLearning::ILearningModelFeatureDescriptor variableDesc) : m_bindingDesc(variableDesc)
    {
        UINT numElements = 0;
        if (variableDesc.Kind() == LearningModelFeatureKind::Tensor)
        {
            InitTensorBinding(variableDesc, numElements);
        }
        else
        {
            ThrowFailure(L"ModelBinding: Binding feature type not implemented");
        }
    }

    winrt::Windows::AI::MachineLearning::ILearningModelFeatureDescriptor GetDesc()
    {
        return m_bindingDesc;
    }

    UINT GetNumElements() const
    {
        return m_numElements;
    }

    UINT GetElementSize() const
    {
        return m_elementSize;
    }

    std::vector<int64_t> GetShapeBuffer()
    {
        return m_shapeBuffer;
    }

    T* GetData()
    {
        return m_dataBuffer.data();
    }

    std::vector<T> GetDataBuffer()
    {
        return m_dataBuffer;
    }

    size_t GetDataBufferSize()
    {
        return m_dataBuffer.size();
    }


private:
    void InitNumElementsAndShape(winrt::Windows::Foundation::Collections::IVectorView<int64_t> * shape, UINT numDimensions, UINT numElements)
    {
        int unknownDim = -1;
        UINT numKnownElements = 1;
        for (UINT dim = 0; dim < numDimensions; dim++)
        {
            INT64 dimSize = shape->GetAt(dim);

            if (dimSize <= 0)
            {
                if (unknownDim == -1)
                {
                    dimSize = 1;
                }
            }
            else
            {
                numKnownElements *= static_cast<UINT>(dimSize);
            }

            m_shapeBuffer.push_back(dimSize);
        }
        m_numElements = numKnownElements;
    }

    void InitTensorBinding(winrt::Windows::AI::MachineLearning::ILearningModelFeatureDescriptor descriptor, UINT numElements)
    {
        auto tensorDescriptor = descriptor.as<winrt::Windows::AI::MachineLearning::TensorFeatureDescriptor>();
        InitNumElementsAndShape(&tensorDescriptor.Shape(), tensorDescriptor.Shape().Size(), 1);
        m_dataBuffer.resize(m_numElements);
    }

    winrt::Windows::AI::MachineLearning::ILearningModelFeatureDescriptor m_bindingDesc;
    std::vector<INT64> m_shapeBuffer;
    UINT m_numElements = 0;
    UINT m_elementSize = 0;
    std::vector<T> m_dataBuffer;
};