#include "pch.h"
#include "DXCoreHelper.h"

using namespace winrt;
using namespace Windows::AI::MachineLearning;
using namespace Windows::Foundation::Collections;

namespace winrt::DXCore_WinRTComponent::implementation
{
    DXCoreHelper::DXCoreHelper()
    {
        check_hresult(DXCoreCreateAdapterFactory(IID_PPV_ARGS(_factory.put())));
    }

    ///<summary>
    /// Uses the DXCore API to specifically target the Intel MyriadX VPU;
    /// returns nullptr if no VPU is found.
    ///</summary>
    Windows::AI::MachineLearning::LearningModelDevice DXCoreHelper::GetDeviceFromVpuAdapter()
    {
        com_ptr<IDXCoreAdapterList> spAdapterList;
        const GUID dxGUIDs[] = { DXCORE_ADAPTER_ATTRIBUTE_D3D12_CORE_COMPUTE };

        check_hresult(_factory->CreateAdapterList(ARRAYSIZE(dxGUIDs), dxGUIDs, IID_PPV_ARGS(&spAdapterList)));

        com_ptr<IDXCoreAdapter> vpuAdapter;
        for (UINT i = 0; i < spAdapterList->GetAdapterCount(); i++)
        {
            com_ptr<IDXCoreAdapter> spAdapter;
            check_hresult(spAdapterList->GetAdapter(i, IID_PPV_ARGS(&spAdapter)));

            DXCoreHardwareID dxCoreHardwareID;
            check_hresult(spAdapter->GetProperty(
                DXCoreAdapterProperty::HardwareID,
                sizeof(dxCoreHardwareID),
                &dxCoreHardwareID));

            if (dxCoreHardwareID.vendorID == 0x8086 && dxCoreHardwareID.deviceID == 0x6200)
            {
                // Use the specific vendor and device IDs for the Intel MyriadX VPU.
                vpuAdapter = spAdapter;
            }
        }

        LearningModelDevice device = nullptr;
        if (vpuAdapter != nullptr)
        {
            device = GetLearningModelDeviceFromAdapter(vpuAdapter.get());
        }

        return device;
    }

    ///<summary>
    /// Uses the DXCore API to select a compute accelerator, which supports compute but not graphics,
    /// i.e. an MCDM adapter such as a VPU. Uses the first valid hardware adapter found; if there are none
    /// returns nullptr.
    ///</summary>
    winrt::Windows::AI::MachineLearning::LearningModelDevice DXCoreHelper::GetDeviceFromComputeOnlyAdapter()
    {
        com_ptr<IDXCoreAdapterList> spAdapterList;
        const GUID dxGUIDs[] = { DXCORE_ADAPTER_ATTRIBUTE_D3D12_CORE_COMPUTE };

        check_hresult(_factory->CreateAdapterList(ARRAYSIZE(dxGUIDs), dxGUIDs, IID_PPV_ARGS(&spAdapterList)));

        com_ptr<IDXCoreAdapter> hwAdapter;
        for (UINT i = 0; i < spAdapterList->GetAdapterCount(); i++)
        {
            com_ptr<IDXCoreAdapter> spAdapter;
            check_hresult(spAdapterList->GetAdapter(i, IID_PPV_ARGS(&spAdapter)));

            // Reject adapters that support both compute and graphics, e.g. GPUs.
            if (spAdapter->IsAttributeSupported(DXCORE_ADAPTER_ATTRIBUTE_D3D12_GRAPHICS))
            {
                continue;
            }

            bool isHardware = false;

            check_hresult(spAdapter->GetProperty(
                DXCoreAdapterProperty::IsHardware,
                sizeof(isHardware),
                &isHardware));

            if (isHardware)
            {
                hwAdapter = spAdapter;
                break;
            }
        }

        LearningModelDevice device = nullptr;
        if (hwAdapter != nullptr)
        {
            device = GetLearningModelDeviceFromAdapter(hwAdapter.get());
        }

        return device;
    }

    ///<summary>
    /// Uses the DXCore API to select a hardware adapter that is capable of both
    /// compute and graphics, i.e. a GPU. Uses the first valid hardware adapter found; if there are none
    /// returns nullptr.
    ///</summary>
    winrt::Windows::AI::MachineLearning::LearningModelDevice DXCoreHelper::GetDeviceFromGraphicsAdapter()
    {
        com_ptr<IDXCoreAdapterList> spAdapterList;
        const GUID dxGUIDs[] = {
            DXCORE_ADAPTER_ATTRIBUTE_D3D12_CORE_COMPUTE,
            DXCORE_ADAPTER_ATTRIBUTE_D3D12_GRAPHICS };

        check_hresult(_factory->CreateAdapterList(ARRAYSIZE(dxGUIDs), dxGUIDs, IID_PPV_ARGS(&spAdapterList)));

        com_ptr<IDXCoreAdapter> hwAdapter;
        for (UINT i = 0; i < spAdapterList->GetAdapterCount(); i++)
        {
            com_ptr<IDXCoreAdapter> spAdapter;
            check_hresult(spAdapterList->GetAdapter(i, IID_PPV_ARGS(&spAdapter)));

            bool isHardware = false;

            check_hresult(spAdapter->GetProperty(
                DXCoreAdapterProperty::IsHardware,
                sizeof(isHardware),
                &isHardware));

            if (isHardware)
            {
                hwAdapter = spAdapter;
                break;
            }
        }

        LearningModelDevice device = nullptr;
        if (hwAdapter != nullptr)
        {
            device = GetLearningModelDeviceFromAdapter(hwAdapter.get());
        }

        return device;
    }

    ///<summary>
    /// Uses the DXCore API to enumerate and return all available hardware adapters that
    /// are capable of at least compute, i.e. both GPUs and compute accelerators.
    /// If no valid hardware adapters are found, returns an empty IVectorView.
    ///</summary>
    IVectorView<LearningModelDevice> DXCoreHelper::GetAllHardwareDevices()
    {
        com_ptr<IDXCoreAdapterList> spAdapterList;
        const GUID dxGUIDs[] = { DXCORE_ADAPTER_ATTRIBUTE_D3D12_CORE_COMPUTE };

        check_hresult(_factory->CreateAdapterList(ARRAYSIZE(dxGUIDs), dxGUIDs, IID_PPV_ARGS(&spAdapterList)));

        auto devices = single_threaded_vector<LearningModelDevice>();
        for (UINT i = 0; i < spAdapterList->GetAdapterCount(); i++)
        {
            com_ptr<IDXCoreAdapter> spAdapter;
            check_hresult(spAdapterList->GetAdapter(i, IID_PPV_ARGS(&spAdapter)));

            bool isHardware = false;

            check_hresult(spAdapter->GetProperty(
                DXCoreAdapterProperty::IsHardware,
                sizeof(isHardware),
                &isHardware));

            if (isHardware)
            {
                LearningModelDevice device = GetLearningModelDeviceFromAdapter(spAdapter.get());
                devices.Append(device);
            }
        }

        return devices.GetView();
    }

    LearningModelDevice DXCoreHelper::GetLearningModelDeviceFromAdapter(IDXCoreAdapter* adapter)
    {
        D3D_FEATURE_LEVEL d3dFeatureLevel = D3D_FEATURE_LEVEL_1_0_CORE;
        D3D12_COMMAND_LIST_TYPE commandQueueType = D3D12_COMMAND_LIST_TYPE_COMPUTE;

        // create D3D12Device
        com_ptr<ID3D12Device> d3d12Device;
        check_hresult(D3D12CreateDevice(adapter, d3dFeatureLevel, __uuidof(ID3D12Device), d3d12Device.put_void()));

        // create D3D12 command queue from device
        com_ptr<ID3D12CommandQueue> d3d12CommandQueue;
        D3D12_COMMAND_QUEUE_DESC commandQueueDesc = {};
        commandQueueDesc.Type = commandQueueType;
        check_hresult(d3d12Device->CreateCommandQueue(&commandQueueDesc, __uuidof(ID3D12CommandQueue), d3d12CommandQueue.put_void()));

        // create LearningModelDevice from command queue
        auto factory = get_activation_factory<LearningModelDevice, ILearningModelDeviceFactoryNative>();
        com_ptr<::IUnknown> spUnkLearningModelDevice;
        check_hresult(factory->CreateFromD3D12CommandQueue(d3d12CommandQueue.get(), spUnkLearningModelDevice.put()));
        return spUnkLearningModelDevice.as<LearningModelDevice>();
    }
}
