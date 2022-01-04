#include "pch.h"
#include "AdapterList.h"
#include "AdapterList.g.cpp"

namespace winrt::WinMLSamplesGalleryNative::implementation
{
    winrt::WinMLSamplesGalleryNative::AdapterList AdapterList::GetAdapters() {
        return winrt::make<AdapterList>();
    }
}