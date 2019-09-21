#pragma once

#include "DXCoreHelper.g.h"

namespace winrt::DXCore_WinRTComponent::implementation
{
    struct DXCoreHelper : DXCoreHelperT<DXCoreHelper>
    {
        DXCoreHelper() = default;

        static winrt::Windows::AI::MachineLearning::LearningModelDevice GetDeviceFromVpuAdapter();
        static winrt::Windows::AI::MachineLearning::LearningModelDevice GetDeviceFromComputeOnlyAdapter();
        static winrt::Windows::AI::MachineLearning::LearningModelDevice GetDeviceFromGraphicsAdapter();
        static winrt::Windows::Foundation::Collections::IVectorView<winrt::Windows::AI::MachineLearning::LearningModelDevice> GetAllHardwareDevices();

    private:
        static winrt::Windows::AI::MachineLearning::LearningModelDevice GetLearningModelDeviceFromAdapter(IDXCoreAdapter* adapter);
    };
}

namespace winrt::DXCore_WinRTComponent::factory_implementation
{
    struct DXCoreHelper : DXCoreHelperT<DXCoreHelper, implementation::DXCoreHelper>
    {
    };
}
