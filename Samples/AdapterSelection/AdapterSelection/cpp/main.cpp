#include "pch.h"
#include "FileHelper.h"

using namespace winrt;
using namespace Windows::Foundation::Collections;
using namespace Windows::AI::MachineLearning;
using namespace Windows::Media;
using namespace Windows::Graphics::Imaging;
using namespace Windows::Storage;
using namespace std;

// globals
vector<string> labels;
string labelsFileName("labels.txt");
hstring modelPath;
hstring imagePath;

// helper functions
void PrintResults(IVectorView<float> results);
bool ParseArgs(int argc, char* argv[]);
LearningModelDevice getLearningModelDeviceFromAdapter(com_ptr<IDXGIAdapter1> spAdapter);

int main(int argc, char* argv[])
{
	init_apartment();

	if (ParseArgs(argc, argv) == false)
	{
		printf("Usage: %s [modelfile] [imagefile]", argv[0]);
		return -1;
	}

	// display all adapters
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
        
        // is a software adapter
        if (pDesc.Flags == DXGI_ADAPTER_FLAG_SOFTWARE || (pDesc.VendorId == 0x1414 && pDesc.DeviceId == 0x8c)) {
            continue;
        } 
        // valid GPU adapter
        else {
            printf("Index: %d, Description: %ls\n", static_cast<int>(validAdapters.size()), pDesc.Description);
            validAdapters.push_back(spAdapter);
        }
    }
    LearningModelDevice device = nullptr;
	if (validAdapters.size() == 0) {
		printf("There are no available adapters, running on CPU...\n");
        device = LearningModelDevice(LearningModelDeviceKind::Cpu);
    }
    else {
        // user selects adapter
        printf("Please enter the index of the adapter you want to use...\n");
        int selectedIndex;
        cin >> selectedIndex;
        while (selectedIndex < 0 || selectedIndex >= static_cast<int>(validAdapters.size())) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            printf("Invalid index, please try again.\n");
            cin >> selectedIndex;
        }
        printf("Selected adapter at index %d\n", selectedIndex);

        device = getLearningModelDeviceFromAdapter(validAdapters.at(selectedIndex));
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
	printf("Loading the image...\n");
	auto imageFrame = FileHelper::LoadImageFile(imagePath);

	// bind the input image
	printf("Binding...\n");
	binding.Bind(model.InputFeatures().GetAt(0).Name(), ImageFeatureValue::CreateFromVideoFrame(imageFrame));
	// temp: bind the output (we don't support unbound outputs yet)
	vector<int64_t> shape({ 1, 1000, 1, 1 });
	hstring outputName = model.OutputFeatures().GetAt(0).Name();
	binding.Bind(outputName, TensorFloat::Create(shape));

	// now run the model
	printf("Running the model...\n");
	ticks = GetTickCount();
	auto results = session.Evaluate(binding, L"RunId");
	ticks = GetTickCount() - ticks;
	printf("model run took %d ticks\n", ticks);

	// get the output
	auto resultTensor = results.Outputs().Lookup(outputName).as<TensorFloat>();
	auto resultVector = resultTensor.GetAsVectorView();
	PrintResults(resultVector);
}

LearningModelDevice getLearningModelDeviceFromAdapter(com_ptr<IDXGIAdapter1> spAdapter) {

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

bool ParseArgs(int argc, char* argv[])
{
	if (argc < 3)
	{
		return false;
	}
	// get the model file
	modelPath = hstring(wstring_to_utf8().from_bytes(argv[1]));
	// get the image file
	imagePath = hstring(wstring_to_utf8().from_bytes(argv[2]));
	// did they pass a fourth arg?

	return true;
}

void PrintResults(IVectorView<float> results)
{
    // load the labels
    auto modulePath = FileHelper::GetModulePath();
    std::string labelsFilePath = std::string(modulePath.begin(), modulePath.end()) + labelsFileName;
    labels = FileHelper::LoadLabels(labelsFilePath);

    vector<pair<float, uint32_t>> sortedResults;
    for (uint32_t i = 0; i < results.Size(); i++) {
        pair<float, uint32_t> curr;
        curr.first = results.GetAt(i);
        curr.second = i;
        sortedResults.push_back(curr);
    }
    std::sort(sortedResults.begin(), sortedResults.end(),
        [](pair<float, uint32_t> const &a, pair<float, uint32_t> const &b) { return a.first > b.first; });

    // Display the result
    for (int i = 0; i < 3; i++)
    {
        pair<float, uint32_t> curr = sortedResults.at(i);
        printf("%s with confidence of %f\n", labels[curr.second].c_str(), curr.first);
    }
}
