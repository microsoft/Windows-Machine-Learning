#pragma once
#include "AdapterList.g.h"

namespace winrt::WinMLSamplesGalleryNative::implementation
{
    struct AdapterList : AdapterListT<AdapterList>
    {
        AdapterList() = default;
        static winrt::com_array<hstring> GetAdapters();
        static winrt::hstring GetAdapterByDriverDescription(winrt::hstring description);
    };
}
namespace winrt::WinMLSamplesGalleryNative::factory_implementation
{
    struct AdapterList : AdapterListT<AdapterList, implementation::AdapterList>
    {
    };
}

