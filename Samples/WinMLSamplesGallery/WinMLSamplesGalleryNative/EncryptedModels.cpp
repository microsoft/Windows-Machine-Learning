#include "pch.h"
#include "EncryptedModels.h"
#include "EncryptedModels.g.cpp"

#include <Windows.h>
#include "resource.h"
#include "WeakBuffer.h"
#include "RandomAccessStream.h"

#include "winrt/Microsoft.AI.MachineLearning.h"

namespace wrl = ::Microsoft::WRL;
namespace details = ::Microsoft::AI::MachineLearning::Details;
namespace abi_wss = ABI::Windows::Storage::Streams;

namespace winrt::WinMLSamplesGalleryNative::implementation
{
static HMODULE GetCurrentModule()
{ // NB: XP+ solution!
    HMODULE hModule = NULL;
    GetModuleHandleEx(
        GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
        (LPCTSTR)GetCurrentModule,
        &hModule);

    return hModule;
}


static winrt::Microsoft::AI::MachineLearning::LearningModel CreateModelFromRandomAccessStreamReferenceFromPath(
    const char* key) {
    auto current_module = GetCurrentModule();

    // Find the resource
    const auto modelResource = FindResource(current_module, MAKEINTRESOURCE(IDR_SQUEEZENET_MODEL), DataFileTypeString);
    if (modelResource == nullptr)
    {
        __fastfail(1);
    }

    // Load the resource
    const auto modelMem = LoadResource(current_module, modelResource);
    if (modelMem == nullptr)
    {
        __fastfail(2);
    }

    // get a byte point to the resource
    const unsigned char* data = static_cast<const unsigned char*>(LockResource(modelMem));
    const auto length = SizeofResource(current_module, modelResource);

    // get start and end pointers
    auto start = reinterpret_cast<const uint8_t*>(data);
    auto end = reinterpret_cast<const uint8_t*>(data + length);

    // wrap bytes in weak buffer
    winrt::com_ptr<abi_wss::IBuffer> ptr;
    wrl::MakeAndInitialize<details::WeakBuffer<uint8_t>>(ptr.put(), start, end);

    // wrap buffer in random access stream
    wrl::ComPtr<ABI::Windows::Storage::Streams::IRandomAccessStreamReference> reference;
    wrl::MakeAndInitialize<::WinMLSamplesGalleryNative::RandomAccessStreamReference>(&reference, ptr.get());

    winrt::Windows::Storage::Streams::IRandomAccessStreamReference random_access_stream;
    winrt::attach_abi(random_access_stream, reference.Detach());
    auto model = winrt::Microsoft::AI::MachineLearning::LearningModel::LoadFromStream(random_access_stream);

    // clean up.
    FreeResource(modelMem);

    return model;
}

winrt::Microsoft::AI::MachineLearning::LearningModel EncryptedModels::LoadEncryptedResource(hstring const& key)
{
    return CreateModelFromRandomAccessStreamReferenceFromPath(winrt::to_string(key).c_str());
}

}
