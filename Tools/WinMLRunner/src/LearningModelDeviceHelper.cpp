#include "LearningModelDeviceHelper.h"
#include "TypeHelper.h"
#include "Common.h"
#include "d3d11.h"
#include "d3dx12.h"
#include <Windows.Graphics.DirectX.Direct3D11.interop.h>
#include "Windows.AI.MachineLearning.Native.h"
#include <codecvt>
#include "OutputHelper.h"
using namespace winrt::Windows::Graphics::DirectX::Direct3D11;

#ifdef DXCORE_SUPPORTED_BUILD
HRESULT CreateDXGIFactory2SEH(void** dxgiFactory)
{
    // Recover from delay-load module failure.
    HRESULT hr;
    __try
    {
        hr = CreateDXGIFactory2(0, __uuidof(IDXGIFactory4), dxgiFactory);
    }
    __except (GetExceptionCode() == VcppException(ERROR_SEVERITY_ERROR, ERROR_MOD_NOT_FOUND)
                  ? EXCEPTION_EXECUTE_HANDLER
                  : EXCEPTION_CONTINUE_SEARCH)
    {
        hr = HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
    }
    return hr;
}
#endif

void PopulateLearningModelDeviceList(CommandLineArgs& args, std::vector<LearningModelDeviceWithMetadata>& deviceList)
{
    std::vector<DeviceType> deviceTypes = args.FetchDeviceTypes();
    std::vector<DeviceCreationLocation> deviceCreationLocations = args.FetchDeviceCreationLocations();
    for (auto deviceType : deviceTypes)
    {
        for (auto deviceCreationLocation : deviceCreationLocations)
        {
            try
            {
#ifdef DXCORE_SUPPORTED_BUILD
                const std::wstring& adapterName = args.GetGPUAdapterName();
#endif
                if (deviceCreationLocation == DeviceCreationLocation::UserD3DDevice && deviceType != DeviceType::CPU)
                {
                    // Enumerate Adapters to pick the requested one.
                    com_ptr<IDXGIFactory6> factory;
                    HRESULT hr = CreateDXGIFactory(__uuidof(IDXGIFactory6), factory.put_void());
                    THROW_IF_FAILED(hr);

                    com_ptr<IDXGIAdapter> adapter;
                    switch (deviceType)
                    {
                        case DeviceType::DefaultGPU:
                            hr = factory->EnumAdapterByGpuPreference(0, DXGI_GPU_PREFERENCE_UNSPECIFIED,
                                                                     __uuidof(IDXGIAdapter), adapter.put_void());
                            break;
                        case DeviceType::MinPowerGPU:
                            hr = factory->EnumAdapterByGpuPreference(0, DXGI_GPU_PREFERENCE_MINIMUM_POWER,
                                                                     __uuidof(IDXGIAdapter), adapter.put_void());
                            break;
                        case DeviceType::HighPerfGPU:
                            hr = factory->EnumAdapterByGpuPreference(0, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
                                                                     __uuidof(IDXGIAdapter), adapter.put_void());
                            break;
                        default:
                            throw hresult(E_INVALIDARG);
                    }
                    THROW_IF_FAILED(hr);

                    // Creating the device on the client and using it to create the video frame and initialize the
                    // session makes sure that everything is on the same device. This usually avoids an expensive
                    // cross-device and cross-videoframe copy via the VideoFrame pipeline.
                    com_ptr<ID3D11Device> d3d11Device;
                    hr = D3D11CreateDevice(adapter.get(), D3D_DRIVER_TYPE_UNKNOWN, nullptr,
                                           D3D11_CREATE_DEVICE_BGRA_SUPPORT, nullptr, 0, D3D11_SDK_VERSION,
                                           d3d11Device.put(), nullptr, nullptr);
                    THROW_IF_FAILED(hr);

                    com_ptr<IDXGIDevice> dxgiDevice;
                    hr = d3d11Device->QueryInterface(__uuidof(IDXGIDevice), dxgiDevice.put_void());
                    THROW_IF_FAILED(hr);

                    com_ptr<IInspectable> inspectableDevice;
                    hr = CreateDirect3D11DeviceFromDXGIDevice(dxgiDevice.get(), inspectableDevice.put());
                    THROW_IF_FAILED(hr);
                    LearningModelDeviceWithMetadata learningModelDeviceWithMetadata =
                        {
                        LearningModelDevice::CreateFromDirect3D11Device(inspectableDevice.as<IDirect3DDevice>()),
                        deviceType,
                        deviceCreationLocation
                        };
                    OutputHelper::PrintLearningModelDevice(learningModelDeviceWithMetadata);
                    deviceList.push_back(learningModelDeviceWithMetadata);
                }
#ifdef DXCORE_SUPPORTED_BUILD
                else if ((TypeHelper::GetWinmlDeviceKind(deviceType) != LearningModelDeviceKind::Cpu) &&
                         !adapterName.empty())
                {
                    com_ptr<IDXCoreAdapterFactory> spFactory;
                    THROW_IF_FAILED(DXCoreCreateAdapterFactory(IID_PPV_ARGS(spFactory.put())));

                    com_ptr<IDXCoreAdapterList> spAdapterList;
                    const GUID dxGUIDs[] = { DXCORE_ADAPTER_ATTRIBUTE_D3D12_CORE_COMPUTE };

                    THROW_IF_FAILED(
                        spFactory->CreateAdapterList(ARRAYSIZE(dxGUIDs), dxGUIDs, IID_PPV_ARGS(spAdapterList.put())));

                    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
                    std::string adapterNameStr = converter.to_bytes(adapterName);
                    com_ptr<IDXCoreAdapter> spAdapter = nullptr;
                    com_ptr<IDXCoreAdapter> currAdapter = nullptr;
                    bool chosenAdapterFound = false;
                    printf("Printing available adapters..\n");
                    for (UINT i = 0; i < spAdapterList->GetAdapterCount(); i++)
                    {
                        THROW_IF_FAILED(spAdapterList->GetAdapter(i, currAdapter.put()));

                        // If the adapter is a software adapter then don't consider it for index selection
                        bool isHardware;
                        size_t driverDescriptionSize;
                        THROW_IF_FAILED(currAdapter->GetPropertySize(DXCoreAdapterProperty::DriverDescription,
                                                                     &driverDescriptionSize));
                        CHAR* driverDescription = new CHAR[driverDescriptionSize];
                        THROW_IF_FAILED(currAdapter->GetProperty(DXCoreAdapterProperty::IsHardware, &isHardware));
                        THROW_IF_FAILED(currAdapter->GetProperty(DXCoreAdapterProperty::DriverDescription,
                                                                 driverDescriptionSize, driverDescription));
                        if (isHardware)
                        {
                            printf("Description: %s\n", driverDescription);
                        }
                        if (!adapterName.empty() && !chosenAdapterFound)
                        {
                            std::string driverDescriptionStr = std::string(driverDescription);
                            std::transform(driverDescriptionStr.begin(), driverDescriptionStr.end(),
                                           driverDescriptionStr.begin(), ::tolower);
                            std::transform(adapterNameStr.begin(), adapterNameStr.end(), adapterNameStr.begin(),
                                           ::tolower);
                            if (strstr(driverDescriptionStr.c_str(), adapterNameStr.c_str()))
                            {
                                chosenAdapterFound = true;
                                spAdapter = currAdapter;
                            }
                        }
                        currAdapter = nullptr;
                        free(driverDescription);
                    }

                    if (spAdapter == nullptr)
                    {
                        throw hresult_invalid_argument(L"ERROR: No matching adapter with given adapter name: " +
                                                       adapterName);
                    }
                    size_t driverDescriptionSize;
                    THROW_IF_FAILED(
                        spAdapter->GetPropertySize(DXCoreAdapterProperty::DriverDescription, &driverDescriptionSize));
                    CHAR* driverDescription = new CHAR[driverDescriptionSize];
                    spAdapter->GetProperty(DXCoreAdapterProperty::DriverDescription, driverDescriptionSize,
                                           driverDescription);
                    printf("Using adapter : %s\n", driverDescription);
                    free(driverDescription);
                    IUnknown* pAdapter = spAdapter.get();
                    com_ptr<IDXGIAdapter> spDxgiAdapter;
                    D3D_FEATURE_LEVEL d3dFeatureLevel = D3D_FEATURE_LEVEL_1_0_CORE;
                    D3D12_COMMAND_LIST_TYPE commandQueueType = D3D12_COMMAND_LIST_TYPE_COMPUTE;

                    // Check if adapter selected has DXCORE_ADAPTER_ATTRIBUTE_D3D12_GRAPHICS attribute selected. If
                    // so, then GPU was selected that has D3D12 and D3D11 capabilities. It would be the most stable
                    // to use DXGI to enumerate GPU and use D3D_FEATURE_LEVEL_11_0 so that image tensorization for
                    // video frames would be able to happen on the GPU.
                    if (spAdapter->IsAttributeSupported(DXCORE_ADAPTER_ATTRIBUTE_D3D12_GRAPHICS))
                    {
                        d3dFeatureLevel = D3D_FEATURE_LEVEL::D3D_FEATURE_LEVEL_11_0;
                        com_ptr<IDXGIFactory4> dxgiFactory4;
                        HRESULT hr;
                        try
                        {
                            hr = CreateDXGIFactory2SEH(dxgiFactory4.put_void());
                        }
                        catch (...)
                        {
                            hr = E_FAIL;
                        }
                        if (hr == S_OK)
                        {
                            // If DXGI factory creation was successful then get the IDXGIAdapter from the LUID
                            // acquired from the selectedAdapter
                            std::cout << "Using DXGI for adapter creation.." << std::endl;
                            LUID adapterLuid;
                            THROW_IF_FAILED(spAdapter->GetProperty(DXCoreAdapterProperty::InstanceLuid, &adapterLuid));
                            THROW_IF_FAILED(dxgiFactory4->EnumAdapterByLuid(adapterLuid, __uuidof(IDXGIAdapter),
                                                                            spDxgiAdapter.put_void()));
                            pAdapter = spDxgiAdapter.get();
                        }
                    }

                    // create D3D12Device
                    com_ptr<ID3D12Device> d3d12Device;
                    THROW_IF_FAILED(
                        D3D12CreateDevice(pAdapter, d3dFeatureLevel, __uuidof(ID3D12Device), d3d12Device.put_void()));

                    // create D3D12 command queue from device
                    com_ptr<ID3D12CommandQueue> d3d12CommandQueue;
                    D3D12_COMMAND_QUEUE_DESC commandQueueDesc = {};
                    commandQueueDesc.Type = commandQueueType;
                    THROW_IF_FAILED(d3d12Device->CreateCommandQueue(&commandQueueDesc, __uuidof(ID3D12CommandQueue),
                                                                    d3d12CommandQueue.put_void()));

                    // create LearningModelDevice from command queue
                    auto factory = get_activation_factory<LearningModelDevice, ILearningModelDeviceFactoryNative>();
                    com_ptr<::IUnknown> spUnkLearningModelDevice;
                    THROW_IF_FAILED(
                        factory->CreateFromD3D12CommandQueue(d3d12CommandQueue.get(), spUnkLearningModelDevice.put()));
                    deviceList.push_back({
                        spUnkLearningModelDevice.as<LearningModelDevice>(),
                        deviceType,
                        deviceCreationLocation
                        });
                }
#endif
                else
                {
                    LearningModelDeviceWithMetadata learningModelDeviceWithMetadata = 
                    { 
                        LearningModelDevice( TypeHelper::GetWinmlDeviceKind(deviceType)),
                                             deviceType, 
                                             deviceCreationLocation 
                    };
                    OutputHelper::PrintLearningModelDevice(learningModelDeviceWithMetadata);
                    deviceList.push_back(learningModelDeviceWithMetadata);
                }
            }
            catch (...)
            {
                printf("Creating LearningModelDevice failed!");
                throw;
            }
        }
    }
}
