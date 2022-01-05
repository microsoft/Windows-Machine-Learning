#include "pch.h"
#include "AdapterList.h"
#include "AdapterList.g.cpp"

namespace winrt::WinMLSamplesGalleryNative::implementation
{
    winrt::hstring AdapterList::GetAdapters() {
        //return winrt::make<AdapterList>();
        hstring w = L"hello world";
        return w;
    }
}