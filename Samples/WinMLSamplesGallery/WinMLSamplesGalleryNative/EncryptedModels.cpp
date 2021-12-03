#include "pch.h"
#include "EncryptedModels.h"
#include "EncryptedModels.g.cpp"

#include <Windows.h>
#include "resource.h"
#include "WeakBuffer.h"
#include "RandomAccessStream.h"

#include "winrt/Microsoft.AI.MachineLearning.h"
#include "winrt/Windows.Security.Cryptography.h"
#include "winrt/Windows.Security.Cryptography.Core.h"
#include "winrt/Windows.Storage.Streams.h"
#include "winrt/Windows.Foundation.h"

// For encryption
// #include <fstream>

namespace wrl = ::Microsoft::WRL;
namespace details = ::Microsoft::AI::MachineLearning::Details;
namespace abi_wss = ABI::Windows::Storage::Streams;
namespace crypto = winrt::Windows::Security::Cryptography;
namespace crypto_core = winrt::Windows::Security::Cryptography::Core;
namespace wss = winrt::Windows::Storage::Streams;

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

template <typename T = void*>
void EncryptToFile(const char* file, crypto_core::CryptographicKey key, wss::IBuffer buffer) {
    // Encrypt
    auto encrypted_model_buffer = crypto_core::CryptographicEngine::Encrypt(key, buffer, nullptr);

    // Get encrypted_model_buffer as IBufferByteAccess
    auto byte_access = encrypted_model_buffer.as<::Windows::Storage::Streams::IBufferByteAccess>();

    // Get buffer as bytes*
    byte* bytes;
    HRESULT hr = byte_access->Buffer(&bytes);
    if (FAILED(hr)) {
        __fastfail(3);
    }

    // Get buffer length
    auto encrypted_length = encrypted_model_buffer.Length();

    // Write the buffer to the file
    std::ofstream file(file, std::ios_base::binary);
    file.write(reinterpret_cast<const char*>(bytes), encrypted_length);
    file.flush();
    file.close();
}

winrt::Microsoft::AI::MachineLearning::LearningModel EncryptedModels::LoadEncryptedResource(hstring const& key)
{
    winrt::Microsoft::AI::MachineLearning::LearningModel model = nullptr;
    auto current_module = GetCurrentModule();

    // Find the resource
    const auto model_resource_info = FindResource(current_module, MAKEINTRESOURCE(IDR_ENCRYPTED_MODEL), DataFileTypeString);
    if (model_resource_info == nullptr)
    {
        __fastfail(1);
    }

    // Load the resource
    const auto model_resource = LoadResource(current_module, model_resource_info);
    if (model_resource == nullptr)
    {
        __fastfail(2);
    }

    // get a byte point to the resource
    const unsigned char* model_bytes = static_cast<const unsigned char*>(LockResource(model_resource));
    const auto model_length = SizeofResource(current_module, model_resource_info);

    try {
        // get start and end pointers
        auto start = reinterpret_cast<const uint8_t*>(model_bytes);
        auto end = reinterpret_cast<const uint8_t*>(model_bytes + model_length);

        // wrap bytes in weak buffer
        winrt::com_ptr<abi_wss::IBuffer> ptr;
        wrl::MakeAndInitialize<details::WeakBuffer<uint8_t>>(ptr.put(), start, end);
        winrt::Windows::Storage::Streams::IBuffer model_buffer;
        winrt::attach_abi(model_buffer, ptr.detach());

        // Create the key
        auto algorithm_name = crypto_core::SymmetricAlgorithmNames::AesEcbPkcs7();
        auto provider = crypto_core::SymmetricKeyAlgorithmProvider::OpenAlgorithm(algorithm_name);
        auto key_buffer = crypto::CryptographicBuffer::ConvertStringToBinary(key, crypto::BinaryStringEncoding::Utf8);
        auto crypto_key = provider.CreateSymmetricKey(key_buffer);

        // Only needed to generate the encrypted model
        // EncryptToFile("encrypted.onnx", crypto_key, model_buffer);

        // Decrypt
        auto decrypted_model_buffer = crypto_core::CryptographicEngine::Decrypt(crypto_key, model_buffer, nullptr);
        auto decrypted_buffer_abi = winrt::com_ptr<abi_wss::IBuffer> {
            decrypted_model_buffer.as<abi_wss::IBuffer>()
        };

        // wrap buffer in random access stream
        wrl::ComPtr<ABI::Windows::Storage::Streams::IRandomAccessStreamReference> reference;
        wrl::MakeAndInitialize<::WinMLSamplesGalleryNative::RandomAccessStreamReference>(&reference, decrypted_buffer_abi.get());
        winrt::Windows::Storage::Streams::IRandomAccessStreamReference random_access_stream;
        winrt::attach_abi(random_access_stream, reference.Detach());

        model = winrt::Microsoft::AI::MachineLearning::LearningModel::LoadFromStream(random_access_stream);
    }
    catch (...) {
        // Hide errors
        model = nullptr;
    }

    // Clean up.
    FreeResource(model_resource);

    return model;
}

}
