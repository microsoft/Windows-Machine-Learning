#pragma once
#include <Windows.Graphics.DirectX.Direct3D11.interop.h>

ColorManagementMode GetColorManagementMode(const LearningModel& model);
void GetHeightAndWidthFromLearningModelFeatureDescriptor(const ILearningModelFeatureDescriptor& modelFeatureDescriptor,
                                                         uint64_t& width, uint64_t& height);

namespace BindingUtilities
{
    ITensor CreateBindableTensor(const ILearningModelFeatureDescriptor& description, const std::wstring& imagePath,
                                 const InputBindingType inputBindingType, const InputDataType inputDataType,
                                 const CommandLineArgs& args, uint32_t iterationNum,
                                 ColorManagementMode colorManagementMode);

    ImageFeatureValue CreateBindableImage(const ILearningModelFeatureDescriptor& featureDescriptor,
                                          const std::wstring& imagePath, InputBindingType inputBindingType,
                                          InputDataType inputDataType, const winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice winrtDevice,
                                          const CommandLineArgs& args, uint32_t iterationNum,
                                          ColorManagementMode colorManagementMode);

    void PrintOrSaveEvaluationResults(const LearningModel& model, const CommandLineArgs& args,
                                      const winrt::Windows::Foundation::Collections::IMapView<hstring, winrt::Windows::Foundation::IInspectable>& results,
                                      OutputHelper& output, int iterationNum);

}