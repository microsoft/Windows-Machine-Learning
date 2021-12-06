#pragma once
#include "EncryptedModels.g.h"

namespace winrt::WinMLSamplesGalleryNative::implementation
{
    struct EncryptedModels : EncryptedModelsT<EncryptedModels>
    {
        EncryptedModels() = default;

        static winrt::Microsoft::AI::MachineLearning::LearningModel LoadEncryptedResource(hstring const& key);
    };
}
namespace winrt::WinMLSamplesGalleryNative::factory_implementation
{
    struct EncryptedModels : EncryptedModelsT<EncryptedModels, implementation::EncryptedModels>
    {
    };
}
