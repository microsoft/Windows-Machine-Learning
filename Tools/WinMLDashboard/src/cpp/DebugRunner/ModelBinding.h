#pragma once
#include "Common.h"

// Stores data and size of input and output bindings.
template< typename T>
class ModelBinding
{
public:
    ModelBinding(winrt::Windows::AI::MachineLearning::ILearningModelFeatureDescriptor variableDesc) : m_bindingDesc(variableDesc)
    { 
        uint32_t numElements = 0;
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

    uint32_t GetNumElements() const
    {
        return m_numElements;
    }

    uint32_t GetElementSize() const
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
    void InitNumElementsAndShape(winrt::Windows::Foundation::Collections::IVectorView<int64_t> * shape, uint32_t numDimensions, uint32_t numElements)
    {
        int unknownDim = -1;
        uint32_t numKnownElements = 1;
        for (uint32_t dim = 0; dim < numDimensions; dim++)
        {
            int64_t dimSize = shape->GetAt(dim);

            if (dimSize <= 0)
            {
                if (unknownDim == -1)
                {
                    dimSize = 1;
                }
            }
            else
            {
                numKnownElements *= static_cast<uint32_t>(dimSize);
            }

            m_shapeBuffer.push_back(dimSize);
        }
        m_numElements = numKnownElements;
    }

    void InitTensorBinding(winrt::Windows::AI::MachineLearning::ILearningModelFeatureDescriptor descriptor, uint32_t numElements)
    {
        auto tensorDescriptor = descriptor.as<winrt::Windows::AI::MachineLearning::TensorFeatureDescriptor>();
        InitNumElementsAndShape(&tensorDescriptor.Shape(), tensorDescriptor.Shape().Size(), 1);
        m_dataBuffer.resize(m_numElements);
    }

    winrt::Windows::AI::MachineLearning::ILearningModelFeatureDescriptor m_bindingDesc;
    std::vector<int64_t> m_shapeBuffer;
    uint32_t m_numElements = 0;
    uint32_t m_elementSize = 0;
    std::vector<T> m_dataBuffer;
};