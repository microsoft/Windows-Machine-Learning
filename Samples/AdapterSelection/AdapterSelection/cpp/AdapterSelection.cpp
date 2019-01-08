#include "AdapterSelection.h"
#include "pch.h"

using namespace winrt;
using namespace Windows::AI::MachineLearning;
using namespace std;

vector<com_ptr<IDXGIAdapter1>> AdapterSelection::EnumerateAdapters(bool excludeSoftwareAdapter) {
    com_ptr<IDXGIFactory1> spFactory;
    CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)(spFactory.put()));
    std::vector <com_ptr<IDXGIAdapter1>> validAdapters;
    for (UINT i = 0; ; ++i) {
        com_ptr<IDXGIAdapter1> spAdapter;
        if (spFactory->EnumAdapters1(i, spAdapter.put()) != S_OK) {
            break;
        }
        DXGI_ADAPTER_DESC1 pDesc;
        spAdapter->GetDesc1(&pDesc);

        // Software adapter
        if (excludeSoftwareAdapter) {
            DXGI_ADAPTER_DESC1 pDesc;
            spAdapter->GetDesc1(&pDesc);
            if (pDesc.Flags == DXGI_ADAPTER_FLAG_SOFTWARE || (pDesc.VendorId == 0x1414 && pDesc.DeviceId == 0x8c))  {
                continue;
            }
        }
        // valid GPU adapter
        validAdapters.push_back(spAdapter);

    }
    return validAdapters;
}

LearningModelDevice AdapterSelection::GetLearningModelDeviceFromAdapter(winrt::com_ptr<IDXGIAdapter1> spAdapter) {
    // create D3D12Device
    com_ptr<IUnknown> spIUnknownAdapter;
    spAdapter->QueryInterface(IID_IUnknown, spIUnknownAdapter.put_void());
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
        get_activation_factory<LearningModelDevice, ILearningModelDeviceFactoryNative>();
    com_ptr<::IUnknown> spLearningDevice;
    dFactory->CreateFromD3D12CommandQueue(spCommandQueue.get(), spLearningDevice.put());
    return spLearningDevice.as<LearningModelDevice>();
}
