#include "pch.h"
#include "FileHelper.h"
#include <winrt/Windows.Foundation.h>
#include <winstring.h>

extern "C"
{
    HRESULT __stdcall OS_RoGetActivationFactory(HSTRING classId, GUID const& iid, void** factory) noexcept;
}

#ifdef _M_IX86
#pragma comment(linker, "/alternatename:_OS_RoGetActivationFactory@12=_RoGetActivationFactory@12")
#else
#pragma comment(linker, "/alternatename:OS_RoGetActivationFactory=RoGetActivationFactory")
#endif

bool starts_with(std::wstring_view value, std::wstring_view match) noexcept
{
    return 0 == value.compare(0, match.size(), match);
}

int32_t __stdcall WINRT_RoGetActivationFactory(void* classId, winrt::guid const& iid, void** factory) noexcept
{
    *factory = nullptr;
    std::wstring_view name{ WindowsGetStringRawBuffer(static_cast<HSTRING>(classId), nullptr),
                            WindowsGetStringLen(static_cast<HSTRING>(classId)) };
    HMODULE library{ nullptr };

    std::string modulePath = FileHelper::GetModulePath();
    std::wstring winmlDllPath = std::wstring(modulePath.begin(), modulePath.end()) + L"Windows.AI.MachineLearning.dll";

    if (starts_with(name, L"Windows.AI.MachineLearning."))
    {
        const wchar_t* libPath = winmlDllPath.c_str();
        library = LoadLibraryW(libPath);
    }
    else
    {
        return OS_RoGetActivationFactory(static_cast<HSTRING>(classId), iid, factory);
    }

    // If the library is not found, get the default one
    if (!library)
    {
        return OS_RoGetActivationFactory(static_cast<HSTRING>(classId), iid, factory);
    }

    using DllGetActivationFactory = HRESULT __stdcall(HSTRING classId, void** factory);
    auto call = reinterpret_cast<DllGetActivationFactory*>(GetProcAddress(library, "DllGetActivationFactory"));

    if (!call)
    {
        HRESULT const hr = HRESULT_FROM_WIN32(GetLastError());
        WINRT_VERIFY(FreeLibrary(library));
        return hr;
    }

    winrt::com_ptr<winrt::Windows::Foundation::IActivationFactory> activation_factory;
    HRESULT const hr = call(static_cast<HSTRING>(classId), activation_factory.put_void());

    if (FAILED(hr))
    {
        WINRT_VERIFY(FreeLibrary(library));
        return hr;
    }

    if (iid != winrt::guid_of<winrt::Windows::Foundation::IActivationFactory>())
    {
        return activation_factory->QueryInterface(iid, factory);
    }

    *factory = activation_factory.detach();
    return S_OK;
}
