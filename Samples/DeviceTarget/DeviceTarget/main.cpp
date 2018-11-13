// main.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"

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
UINT deviceIndex;
hstring modelPath;
hstring imagePath;

// helper functions
string GetModulePath();
void LoadLabels();
VideoFrame LoadImageFile(hstring filePath);
void PrintResults(IVectorView<float> results);
bool ParseArgs(int argc, char* argv[]);

int main(int argc, char* argv[])
{

	{
		init_apartment();

		if (ParseArgs(argc, argv) == false)
		{
			printf("Usage: %s [modelfile] [imagefile]", argv[0]);
			return -1;
		}

	
		// display all adapters
		com_ptr<IDXGIFactory> spFactory;
		CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)(spFactory.put()));
		com_ptr<IDXGIAdapter> spAdapter;

		UINT i;

		for (i = 0; spFactory->EnumAdapters(i, spAdapter.put()) != DXGI_ERROR_NOT_FOUND ; ++i)
		{
			DXGI_ADAPTER_DESC pDesc;
			spAdapter->GetDesc(&pDesc);
			printf("Index: %d, Description: ", i);
			wcout << pDesc.Description << endl;
			spAdapter = nullptr;
		}

		if (i == 0) {
			printf("There are no available adapters\n");
			return -1;
		}

		// user selects adapter
		int selectedIndex;
		printf("Please enter the index of the adapter you want to use...\n");
		cin >> selectedIndex;

		while (selectedIndex < 0 || selectedIndex >= i) {
			printf("Invalid index, please try again.\n");
			cin >> selectedIndex;
		}

		printf("Selected adapter at index %d\n", selectedIndex);

		// create D3D12Device
		com_ptr<IUnknown> spIUnknownAdapter;

		spFactory->EnumAdapters(selectedIndex, spAdapter.put());

		spAdapter->QueryInterface(IID_IUnknown, spIUnknownAdapter.put_void());
		com_ptr<ID3D12Device> spD3D12Device;
		D3D12CreateDevice(spIUnknownAdapter.get(), D3D_FEATURE_LEVEL_12_0, _uuidof(ID3D12Device), spD3D12Device.put_void());

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

		// load the model
		printf("Loading modelfile '%ws' on the selected device\n", modelPath.c_str());
		DWORD ticks = GetTickCount();
		auto model = LearningModel::LoadFromFilePath(modelPath);
		ticks = GetTickCount() - ticks;
		printf("model file loaded in %d ticks\n", ticks);

		// now create a session and binding
		//LearningModelSession session(model, spLearningDevice.as<LearningModelDevice>());
		LearningModelSession session(model);
		printf("session initialized\n");
		LearningModelBinding binding(session);

		// load the image
		printf("Loading the image...\n");
		auto imageFrame = LoadImageFile(imagePath);

		// bind the input image
		printf("Binding...\n");
		binding.Bind(L"data_0", ImageFeatureValue::CreateFromVideoFrame(imageFrame));
		// temp: bind the output (we don't support unbound outputs yet)
		vector<int64_t> shape({ 1, 1000, 1, 1 });
		binding.Bind(L"softmaxout_1", TensorFloat::Create(shape));

		// now run the model
		printf("Running the model...\n");
		ticks = GetTickCount();
		auto results = session.Evaluate(binding, L"RunId");
		ticks = GetTickCount() - ticks;
		printf("model run took %d ticks\n", ticks);

		// get the output
		auto resultTensor = results.Outputs().Lookup(L"softmaxout_1").as<TensorFloat>();
		auto resultVector = resultTensor.GetAsVectorView();
		PrintResults(resultVector);
	}
	
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
	if (argc >= 4)
	{
		deviceIndex = atoi(argv[3]);
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
		printf("failed to load the %s file.  Make sure it exists in the same folder as the app\r\n", labelsFileName.c_str());
		exit(EXIT_FAILURE);
	}

	std::string s;
	while (std::getline(labelFile, s, ','))
	{
		int labelValue = atoi(s.c_str());
		if (labelValue >= labels.size())
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
		printf("failed to load the image file, make sure you are using fully qualified paths\r\n");
		exit(EXIT_FAILURE);
	}
}

void PrintResults(IVectorView<float> results)
{
	// load the labels
	LoadLabels();
	// Find the top 3 probabilities
	vector<float> topProbabilities(3);
	vector<int> topProbabilityLabelIndexes(3);
	// SqueezeNet returns a list of 1000 options, with probabilities for each, loop through all
	for (uint32_t i = 0; i < results.Size(); i++)
	{
		// is it one of the top 3?
		for (int j = 0; j < 3; j++)
		{
			if (results.GetAt(i) > topProbabilities[j])
			{
				topProbabilityLabelIndexes[j] = i;
				topProbabilities[j] = results.GetAt(i);
				break;
			}
		}
	}
	// Display the result
	for (int i = 0; i < 3; i++)
	{
		printf("%s with confidence of %f\n", labels[topProbabilityLabelIndexes[i]].c_str(), topProbabilities[i]);
	}
}

int32_t WINRT_CALL WINRT_CoIncrementMTAUsage(void** cookie) noexcept
{
	return CoIncrementMTAUsage((CO_MTA_USAGE_COOKIE*)cookie);
}
