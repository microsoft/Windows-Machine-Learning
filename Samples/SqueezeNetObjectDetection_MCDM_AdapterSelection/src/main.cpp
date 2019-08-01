#include "pch.h"

using namespace winrt;
using namespace Windows::Foundation::Collections;
using namespace Windows::AI::MachineLearning;
using namespace Windows::Media;
using namespace Windows::Graphics::Imaging;
using namespace Windows::Storage;
using namespace std;

#define THROW_IF_FAILED(hr)                                                                                            \
    {                                                                                                                  \
        if (FAILED(hr))                                                                                                \
            throw hresult_error(hr);                                                                                   \
    }

// helper functions
string GetModulePath();
void LoadLabels();
VideoFrame LoadImageFile(hstring filePath);
void PrintResults(IVectorView<float> results);
bool ParseArgs(int argc, char* argv[]);
LearningModelDevice GetLearningModelDeviceFromAdapter(IDXCoreAdapter* spAdapter);

// globals
vector<string> labels;
string labelsFileName("labels.txt");
// Many compute-only adapters, including the MyriadX, are optimized for lower precision ML models.
// In this case, we default to an FP16 version of SqueezeNet
hstring modelPath = to_hstring(GetModulePath() + "SqueezeNet_fp16.onnx");
hstring imagePath = to_hstring(GetModulePath() + "kitten_224.png");
bool selectAdapter = false;

int main(int argc, char* argv[]) try
{
    init_apartment();
    if (ParseArgs(argc, argv) == false)
    {
        return -1;
    }
    com_ptr<IDXCoreAdapterFactory> spFactory;
    THROW_IF_FAILED(DXCoreCreateAdapterFactory(IID_PPV_ARGS(spFactory.put())));

    com_ptr<IDXCoreAdapterList> spAdapterList;
    const GUID dxGUIDs[] = { DXCORE_ADAPTER_ATTRIBUTE_D3D12_CORE_COMPUTE };

    THROW_IF_FAILED(spFactory->CreateAdapterList(ARRAYSIZE(dxGUIDs), dxGUIDs, spAdapterList.put()));

    CHAR driverDescription[128];
    std::map<int, com_ptr<IDXCoreAdapter>> validAdapters;
    IDXCoreAdapter* vpuAdapter = nullptr;
    for (UINT i = 0; i < spAdapterList->GetAdapterCount(); i++)
    {
        com_ptr<IDXCoreAdapter> spAdapter;
        THROW_IF_FAILED(spAdapterList->GetAdapter(i, IID_PPV_ARGS(&spAdapter)));
        // If the adapter is a software adapter then don't consider it
        bool isHardware;
        DXCoreHardwareID dxCoreHardwareID;
        THROW_IF_FAILED(spAdapter->GetProperty(DXCoreAdapterProperty::IsHardware, sizeof(isHardware), &isHardware));
        THROW_IF_FAILED(spAdapter->GetProperty(DXCoreAdapterProperty::HardwareID, sizeof(dxCoreHardwareID), &dxCoreHardwareID));

        if (isHardware && (dxCoreHardwareID.vendorID != 0x1414 || dxCoreHardwareID.deviceID != 0x8c))
        {
            if (dxCoreHardwareID.vendorID == 0x8086 && dxCoreHardwareID.deviceID == 0x6200)
            {
                // Use the specific vendor and device IDs for the Intel MyriadX VPU.
                vpuAdapter = spAdapter.get();
            }
            THROW_IF_FAILED(spAdapter->GetProperty(DXCoreAdapterProperty::DriverDescription, sizeof(driverDescription), driverDescription));
            printf("Index: %d, Description: %s\n", i, driverDescription);
            validAdapters[i] = spAdapter;
        }
    }

    LearningModelDevice device = nullptr;
    if (validAdapters.size() == 0)
    {
        printf("There are no available adapters, running on CPU...\n");
        device = LearningModelDevice(LearningModelDeviceKind::Cpu);
    }
    else
    {
        IDXCoreAdapter* chosenAdapter = nullptr;
        if (selectAdapter)
        {
            // user selects adapter
            printf("Please enter the index of the adapter you want to use...\n");
            int selectedIndex;
            while (!(cin >> selectedIndex) || validAdapters.find(selectedIndex) == validAdapters.end())
            {
                cin.clear();
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                printf("Invalid index, please try again.\n");
            }
            com_ptr<IDXCoreAdapter> selectedAdapter;
            THROW_IF_FAILED(spAdapterList->GetAdapter(selectedIndex, IID_PPV_ARGS(&selectedAdapter)));
            THROW_IF_FAILED(selectedAdapter->GetProperty(DXCoreAdapterProperty::DriverDescription, sizeof(driverDescription), driverDescription));
            printf("Selected adapter at index %d, description: %s\n", selectedIndex, driverDescription);
            chosenAdapter = validAdapters[selectedIndex].get();
        }
        else
        {
            // VPU adapter automatically chosen
            if (vpuAdapter != nullptr)
            {
                chosenAdapter = vpuAdapter;
            }
            else
            {
                throw hresult_invalid_argument(L"Default behavior uses VPU adapter, but not found!");
            }
        }
        try
        {
            device = GetLearningModelDeviceFromAdapter(chosenAdapter);
        }
        catch (const hresult_error& hr)
        {
            wprintf(hr.message().c_str());
            printf("Couldn't create Learning Model Device from selected adapter!!\n");
            throw;
        }
        printf("Successfully created LearningModelDevice with selected adapter\n");
    }

    // load the model
    printf("Loading modelfile '%ws' on the selected device\n", modelPath.c_str());
    DWORD ticks = GetTickCount();
    auto model = LearningModel::LoadFromFilePath(modelPath);
    ticks = GetTickCount() - ticks;
    printf("model file loaded in %d ticks\n", ticks);

    // now create a session and binding
    LearningModelSession session(model, device);
    LearningModelBinding binding(session);

    // load the image
    printf("Loading the image: '%ws' ...\n", imagePath.c_str());
    auto imageFrame = LoadImageFile(imagePath);

    // bind the input image
    printf("Binding...\n");
    binding.Bind(model.InputFeatures().GetAt(0).Name(), ImageFeatureValue::CreateFromVideoFrame(imageFrame));

    // now run the model
    printf("Running the model...\n");
    ticks = GetTickCount();
    auto results = session.Evaluate(binding, L"RunId");
    ticks = GetTickCount() - ticks;
    printf("model run took %d ticks\n", ticks);

    // get the output
    auto resultTensor = results.Outputs().Lookup(model.OutputFeatures().GetAt(0).Name()).as<TensorFloat16Bit>();
    auto resultVector = resultTensor.GetAsVectorView();
    PrintResults(resultVector);
    return EXIT_SUCCESS;
}
catch (const hresult_error& error)
{
    wprintf(error.message().c_str());
    return error.code();
}
catch (const std::exception& error)
{
    printf(error.what());
    return EXIT_FAILURE;
}
catch (...)
{
    printf("Unknown exception occurred.");
    return EXIT_FAILURE;
}

LearningModelDevice GetLearningModelDeviceFromAdapter(IDXCoreAdapter* spAdapter)
{

    IUnknown* pAdapter = spAdapter;
    com_ptr<IDXGIAdapter> spDxgiAdapter;
    D3D_FEATURE_LEVEL d3dFeatureLevel = D3D_FEATURE_LEVEL_1_0_CORE;
    D3D12_COMMAND_LIST_TYPE commandQueueType = D3D12_COMMAND_LIST_TYPE_COMPUTE;

    // Check if adapter selected has DXCORE_ADAPTER_ATTRIBUTE_D3D12_GRAPHICS attribute selected. If so,
    // then GPU was selected that has D3D12 and D3D11 capabilities. It would be the most stable to
    // use DXGI to enumerate GPU and use D3D_FEATURE_LEVEL_11_0 so that image tensorization for
    // video frames would be able to happen on the GPU.
    if (spAdapter->IsAttributeSupported(DXCORE_ADAPTER_ATTRIBUTE_D3D12_GRAPHICS))
    {
        d3dFeatureLevel = D3D_FEATURE_LEVEL::D3D_FEATURE_LEVEL_11_0;
        com_ptr<IDXGIFactory4> dxgiFactory4;
        THROW_IF_FAILED(CreateDXGIFactory2(0, __uuidof(IDXGIFactory4), dxgiFactory4.put_void()));

        // If DXGI factory creation was successful then get the IDXGIAdapter from the LUID acquired from the spAdapter
        LUID adapterLuid;
        THROW_IF_FAILED(spAdapter->GetProperty(DXCoreAdapterProperty::InstanceLuid, sizeof(adapterLuid), &adapterLuid));
        THROW_IF_FAILED(dxgiFactory4->EnumAdapterByLuid(adapterLuid, __uuidof(IDXGIAdapter), spDxgiAdapter.put_void()));
        pAdapter = spDxgiAdapter.get();
    }

    // create D3D12Device
    com_ptr<ID3D12Device> d3d12Device;
    THROW_IF_FAILED(D3D12CreateDevice(pAdapter, d3dFeatureLevel, __uuidof(ID3D12Device), d3d12Device.put_void()));

    // create D3D12 command queue from device
    com_ptr<ID3D12CommandQueue> d3d12CommandQueue;
    D3D12_COMMAND_QUEUE_DESC commandQueueDesc = {};
    commandQueueDesc.Type = commandQueueType;
    THROW_IF_FAILED(d3d12Device->CreateCommandQueue(&commandQueueDesc, __uuidof(ID3D12CommandQueue), d3d12CommandQueue.put_void()));

    // create LearningModelDevice from command queue
    auto factory = get_activation_factory<LearningModelDevice, ILearningModelDeviceFactoryNative>();
    com_ptr<::IUnknown> spUnkLearningModelDevice;
    THROW_IF_FAILED(factory->CreateFromD3D12CommandQueue(d3d12CommandQueue.get(), spUnkLearningModelDevice.put()));
    return spUnkLearningModelDevice.as<LearningModelDevice>();
}

bool ParseArgs(int argc, char* argv[])
{
    for (int i = 1; i < argc; i++)
    {
        string arg(argv[i]);
        if ((_stricmp(arg.c_str(), "-Model") == 0) && i + 1 < argc)
        {
            modelPath = hstring(wstring_to_utf8().from_bytes(argv[++i]));
        }
        else if ((_stricmp(arg.c_str(), "-Image") == 0) && i + 1 < argc)
        {
            imagePath = hstring(wstring_to_utf8().from_bytes(argv[++i]));
        }
        else if ((_stricmp(arg.c_str(), "-SelectAdapter") == 0))
        {
            selectAdapter = true;
        }
        else
        {
            std::cout << "SqueezeNetObjectDetection_MCDM_AdapterSelection.exe [options]" << std::endl;
            std::cout << "options: " << std::endl;
            std::cout << "  -Model <full path to model>: Model Path (Only FP16 models)" << std::endl;
            std::cout << "  -Image <full path to image>: Image Path" << std::endl;
            std::cout << "  -SelectAdapter : Toggle select adapter functionality to select the device to run sample on." << std::endl;
            return false;
        }
    }
    return true;
}

string GetModulePath()
{
    string val;
    char modulePath[MAX_PATH] = {};
    GetModuleFileNameA(NULL, modulePath, ARRAYSIZE(modulePath));
    char drive[_MAX_DRIVE];
    char dir[_MAX_DIR];
    char filename[_MAX_FNAME];
    char ext[_MAX_EXT];
    _splitpath_s(modulePath, drive, _MAX_DRIVE, dir, _MAX_DIR, filename, _MAX_FNAME, ext, _MAX_EXT);

    val = drive;
    val += dir;
    return val;
}

void LoadLabels()
{
    // Parse labels from labels file.  We know the file's entries are already sorted in order.
    std::string labelsFilePath = GetModulePath() + labelsFileName;
    ifstream labelFile(labelsFilePath, ifstream::in);
    if (labelFile.fail())
    {
        printf("failed to load the %s file.  Make sure it exists in the same folder as the app\r\n",
               labelsFileName.c_str());
        throw hresult_invalid_argument();
    }

    std::string s;
    while (std::getline(labelFile, s, ','))
    {
        int labelValue = atoi(s.c_str());
        if (labelValue >= static_cast<int>(labels.size()))
        {
            labels.resize(labelValue + 1);
        }
        std::getline(labelFile, s);
        labels[labelValue] = s;
    }
}

VideoFrame LoadImageFile(hstring filePath)
{
    try
    {
        // open the file
        StorageFile file = StorageFile::GetFileFromPathAsync(filePath).get();
        // get a stream on it
        auto stream = file.OpenAsync(FileAccessMode::Read).get();
        // Create the decoder from the stream
        BitmapDecoder decoder = BitmapDecoder::CreateAsync(stream).get();
        // get the bitmap
        SoftwareBitmap softwareBitmap = decoder.GetSoftwareBitmapAsync().get();
        // load a videoframe from it
        VideoFrame inputImage = VideoFrame::CreateWithSoftwareBitmap(softwareBitmap);
        // all done
        return inputImage;
    }
    catch (...)
    {
        printf("failed to load the image file, make sure that file path (\"%ws\") is fully qualified and exists\r\n", imagePath.c_str());
        throw;
    }
}

void PrintResults(IVectorView<float> results)
{
    // load the labels
    LoadLabels();

    vector<pair<float, uint32_t>> sortedResults;
    for (uint32_t i = 0; i < results.Size(); i++)
    {
        pair<float, uint32_t> curr;
        curr.first = results.GetAt(i);
        curr.second = i;
        sortedResults.push_back(curr);
    }
    std::sort(sortedResults.begin(), sortedResults.end(),
              [](pair<float, uint32_t> const& a, pair<float, uint32_t> const& b) { return a.first > b.first; });

    // Display the result
    for (int i = 0; i < 3; i++)
    {
        pair<float, uint32_t> curr = sortedResults.at(i);
        printf("%s with confidence of %f\n", labels[curr.second].c_str(), curr.first);
    }
}

int32_t WINRT_CALL WINRT_CoIncrementMTAUsage(void** cookie) noexcept
{
    return CoIncrementMTAUsage(reinterpret_cast<CO_MTA_USAGE_COOKIE*>(cookie));
}