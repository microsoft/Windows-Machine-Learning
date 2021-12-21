#pragma once
#include "AdapterList.g.h"

namespace winrt::WinMLSamplesGalleryNative::implementation
{
    struct AdapterList : AdapterListT<AdapterList>
    {
        AdapterList() = default;
        static void GetAdapters(hstring const& path);
    };
}
namespace winrt::WinMLSamplesGalleryNative::factory_implementation
{
    struct AdapterList : AdapterListT<AdapterList, implementation::AdapterList>
    {
    };
}

