#include "pch.h"
#include "AdapterList.h"
#include "AdapterList.g.cpp"
#include <initguid.h>
#include <dxcore.h>

namespace winrt::WinMLSamplesGalleryNative::implementation
{
    winrt::com_array<hstring> AdapterList::GetAdapters() {
        //return winrt::make<AdapterList>();

        winrt::com_ptr<IDXCoreAdapterFactory> adapterFactory;
        winrt::check_hresult(::DXCoreCreateAdapterFactory(adapterFactory.put()));
        winrt::com_ptr<IDXCoreAdapterList> d3D12CoreComputeAdapters;
        GUID attributes[]{ DXCORE_ADAPTER_ATTRIBUTE_D3D12_CORE_COMPUTE };
        winrt::check_hresult(
            adapterFactory->CreateAdapterList(_countof(attributes),
                attributes,
                d3D12CoreComputeAdapters.put()));

        const uint32_t count{ d3D12CoreComputeAdapters->GetAdapterCount() };
        com_array<hstring> driver_descriptions = com_array<hstring>(count);
        for (uint32_t i = 0; i < count; ++i)
        {
            winrt::com_ptr<IDXCoreAdapter> candidateAdapter;
            winrt::check_hresult(
                d3D12CoreComputeAdapters->GetAdapter(i, candidateAdapter.put()));
            CHAR description[128];
            candidateAdapter->GetProperty(DXCoreAdapterProperty::DriverDescription, sizeof(description), description);
            driver_descriptions[i] = to_hstring(description);
        }

 /*       hstring str1 = L"string1";
        hstring str2 = L"string2";
        hstring str3 = L"string3";

        com_array<hstring> strings = com_array<hstring>(3);
        strings[0] = str1;
        strings[1] = str2;
        strings[2] = str3;*/

        return driver_descriptions;
    }
}