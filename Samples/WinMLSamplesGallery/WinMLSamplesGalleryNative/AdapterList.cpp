#include "pch.h"
#include "AdapterList.h"
#include "AdapterList.g.cpp"
#include <initguid.h>
#include <dxcore.h>
#include "d3d12.h"
#include "winrt/Microsoft.AI.MachineLearning.h"
#include "Microsoft.AI.MachineLearning.Native.h"

namespace winrt::WinMLSamplesGalleryNative::implementation
{
    winrt::com_array<hstring> AdapterList::GetAdapters() {
        // Create an adapter factory
        winrt::com_ptr<IDXCoreAdapterFactory> adapterFactory;
        winrt::check_hresult(::DXCoreCreateAdapterFactory(adapterFactory.put()));

        // Retrieve an adapter list
        winrt::com_ptr<IDXCoreAdapterList> d3D12CoreComputeAdapters;
        GUID attributes[]{ DXCORE_ADAPTER_ATTRIBUTE_D3D12_CORE_COMPUTE };
        winrt::check_hresult(
            adapterFactory->CreateAdapterList(_countof(attributes),
                attributes,
                d3D12CoreComputeAdapters.put()));

        // Get the adapter descriptions from the adapters in the list
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

        return driver_descriptions;
    }

    winrt::Microsoft::AI::MachineLearning::LearningModelDevice AdapterList::CreateLearningModelDeviceFromAdapter(winrt::hstring description) {
        // Create an adapter factory
        winrt::com_ptr<IDXCoreAdapterFactory> adapterFactory;
        winrt::check_hresult(::DXCoreCreateAdapterFactory(adapterFactory.put()));

        // Retrieve an adapter list
        winrt::com_ptr<IDXCoreAdapterList> d3D12CoreComputeAdapters;
        GUID attributes[]{ DXCORE_ADAPTER_ATTRIBUTE_D3D12_CORE_COMPUTE };
        winrt::check_hresult(
            adapterFactory->CreateAdapterList(_countof(attributes),
                attributes,
                d3D12CoreComputeAdapters.put()));

        // Find the adapter in the list with the matching description
        const uint32_t count{ d3D12CoreComputeAdapters->GetAdapterCount() };
        int found_driver = 0;
        for (uint32_t i = 0; i < count; ++i)
        {
            winrt::com_ptr<IDXCoreAdapter> candidateAdapter;
            winrt::check_hresult(
                d3D12CoreComputeAdapters->GetAdapter(i, candidateAdapter.put()));
            CHAR driver_description[128];
            candidateAdapter->GetProperty(DXCoreAdapterProperty::DriverDescription, sizeof(driver_description), driver_description);
            hstring hstr_driver_description = to_hstring(driver_description);

            if (description == hstr_driver_description) {
                found_driver = 1;

                // create D3D12Device
                com_ptr<IUnknown> spIUnknownAdapter;
                candidateAdapter->QueryInterface(IID_IUnknown, spIUnknownAdapter.put_void());
                com_ptr<ID3D12Device> spD3D12Device;
                D3D12CreateDevice(spIUnknownAdapter.get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), spD3D12Device.put_void());

                // create D3D12 command queue from device
                D3D12_COMMAND_QUEUE_DESC queueDesc = {};
                queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
                queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
                com_ptr<ID3D12CommandQueue> spCommandQueue;
                spD3D12Device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(spCommandQueue.put()));

                // create LearningModelDevice from command queue	
                com_ptr<ILearningModelDeviceFactoryNative> dFactory =
                    get_activation_factory<winrt::Microsoft::AI::MachineLearning::LearningModelDevice, ILearningModelDeviceFactoryNative>();
                com_ptr<::IUnknown> spLearningDevice;
                dFactory->CreateFromD3D12CommandQueue(spCommandQueue.get(), spLearningDevice.put());
                return spLearningDevice.as<winrt::Microsoft::AI::MachineLearning::LearningModelDevice>();
            }
        }
    }
}