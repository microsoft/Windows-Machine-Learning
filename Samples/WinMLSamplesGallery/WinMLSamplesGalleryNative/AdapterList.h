#pragma once
#include "AdapterList.g.h"

namespace winrt::WinMLSamplesGalleryNative::implementation
{
    struct AdapterList : AdapterListT<AdapterList>
    {
        AdapterList() = default;
        static winrt::WinMLSamplesGalleryNative::AdapterList GetAdapters();
    };
}
namespace winrt::WinMLSamplesGalleryNative::factory_implementation
{
    struct AdapterList : AdapterListT<AdapterList, implementation::AdapterList>
    {
    };
}

