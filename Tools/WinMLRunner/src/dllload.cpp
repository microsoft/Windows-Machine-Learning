#include "FileHelper.h"
#include <winrt/Windows.Foundation.h>
#include <winstring.h>
#include "Common.h"
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
    std::wstring_view name{ WindowsGetStringRawBuffer(static_cast<HSTRING>(classId), nullptr), WindowsGetStringLen(static_cast<HSTRING>(classId)) };
    HMODULE library{ nullptr };

    if (starts_with(name, L"Windows.AI.MachineLearning.") && !g_loadWinMLDllPath.empty()) //If we want to load Windows.Ai.MachineLearning.dll from local path
    {
            library = LoadLibraryW(g_loadWinMLDllPath.c_str());
            if (!library)
            {
                std::cout << "ERROR: Loading Windows.AI.MachineLearning.dll from local failed! Check if it is contained in the path that was specified." << std::endl;
                return HRESULT_FROM_WIN32(GetLastError());
            }
    }
    else //If we don't want to load Windows.Ai.MachineLearning.dll from local path, then it will go through the OS normally.
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
