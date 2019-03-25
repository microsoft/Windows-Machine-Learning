#include "pch.h"
#include "TensorConvertor.h"
using namespace winrt;
using namespace Windows::Foundation;
using namespace Windows::AI::MachineLearning;
using namespace Windows::Foundation::Collections;
using namespace Windows::Graphics::Imaging;
using namespace Windows::Media;
using namespace Windows::Storage;
using namespace Windows::Storage::Streams;

using namespace std;

EXTERN_C IMAGE_DOS_HEADER __ImageBase;

// Global variables
LearningModel model = nullptr;
LearningModelSession session = nullptr;
LearningModelBinding binding = nullptr;

const wstring modulePath{ TensorizationHelper::GetModulePath() };

// Forward declarations
void LoadModel(hstring modelPath);
VideoFrame LoadImageFile(hstring filePath);
void BindModel(
    VideoFrame imageFrame,
    string deviceName = "default");
ImageFeatureValue EvaluateModel();
void SaveOutputToDisk(
    ImageFeatureValue resultTensor,
    hstring outputDataImageFileName);

int main()
{
    init_apartment();
    const hstring modelPath = static_cast<hstring>(modulePath.c_str()) + L"fns-candy.onnx";
    const hstring imagePath = static_cast<hstring>(modulePath.c_str()) + L"fish_720.png";

    // The second parameter of BindModel specifies manually tensorization on which device.
    // Mannually-tensorization from CPU
    printf("Mannually-tensorization from CPU\n");
    LoadModel(modelPath);
    VideoFrame imageFrame = LoadImageFile(imagePath);
    BindModel(imageFrame, "CPU");
    ImageFeatureValue output = EvaluateModel();
    SaveOutputToDisk(output, L"output_cpu.png");

    // Mannually-tensorization from GPU
    printf("\n\nMannually-tensorization from GPU\n");
    LoadModel(modelPath);
    imageFrame = LoadImageFile(imagePath);
    BindModel(imageFrame, "GPU");
    output = EvaluateModel();
    SaveOutputToDisk(output, L"output_gpu.png");
}

void LoadModel(hstring modelPath)
{
    // load the model
    printf("Loading modelfile '%ws'\n", modelPath.c_str());
    DWORD ticks = GetTickCount();
    try
    {
        model = LearningModel::LoadFromFilePath(modelPath);
    }
    catch (...)
    {
        printf("failed to load model, make sure you gave the right path\r\n");
        exit(EXIT_FAILURE);
    }
    ticks = GetTickCount() - ticks;
    printf("model file loaded in %d ticks\n", ticks);
}

VideoFrame LoadImageFile(hstring filePath)
{
    printf("Loading the image...\n");
    DWORD ticks = GetTickCount();
    VideoFrame inputImage = nullptr;

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
        inputImage = VideoFrame::CreateWithSoftwareBitmap(softwareBitmap);
    }
    catch (...)
    {
        printf("failed to load the image file, make sure you are using fully qualified paths\r\n");
        exit(EXIT_FAILURE);
    }

    ticks = GetTickCount() - ticks;
    printf("image file loaded in %d ticks\n", ticks);
    // all done
    return inputImage;
}

void BindModel(
    VideoFrame imageFrame,
    string deviceName)
{
    printf("Binding the model...\n");
    DWORD ticks = GetTickCount();

    LearningModelDeviceKind deviceKind;
    TensorFloat inputTensor = nullptr;
    // now create a session and binding
    if ("GPU" == deviceName)
    {
        deviceKind = LearningModelDeviceKind::DirectX;
        inputTensor = TensorizationHelper::SoftwareBitmapToDX12Tensor(imageFrame.SoftwareBitmap());
    }
    else 
    {
        deviceKind = LearningModelDeviceKind::Default;
        inputTensor = TensorizationHelper::SoftwareBitmapToSoftwareTensor(imageFrame.SoftwareBitmap());
    }
    session = LearningModelSession{ model, deviceKind};
    binding = LearningModelBinding{ session };

    // bind the intput image
    
    binding.Bind(model.InputFeatures().First().Current().Name(), inputTensor);

    ticks = GetTickCount() - ticks;
    printf("Model bound in %d ticks\n", ticks);
}

ImageFeatureValue EvaluateModel()
{
    // now run the model
    printf("Running the model...\n");
    DWORD ticks = GetTickCount();

    auto results = session.EvaluateAsync(binding, L"").get();

    ticks = GetTickCount() - ticks;
    printf("model run took %d ticks\n", ticks);

    // get the output
    auto resultTensor = results.Outputs().Lookup(model.OutputFeatures().First().Current().Name()).as<ImageFeatureValue>();

    return resultTensor;
}

void SaveOutputToDisk(
    ImageFeatureValue resultTensor,
    hstring outputDataImageFileName)
{
    // save the output to disk

	// try and see if we can output the pngs to the more visible folder where the vcxproj file is located
	// otherwise output next to the executable
	std::wstring outputPath = modulePath;
	std::wstring filename{ TensorizationHelper::GetFileName() };
	int32_t i = modulePath.find(filename);

	if (i != std::wstring::npos) {
		StorageFolder parentFolder = StorageFolder::GetFolderFromPathAsync(outputPath.substr(0, i + wcslen(filename.c_str()))).get();
		if (parentFolder.TryGetItemAsync(filename).get() != nullptr) {
			outputPath = outputPath.substr(0, i + wcslen(filename.c_str())) + L"\\" + filename + L"\\";
		}
	}

    StorageFolder currentfolder = StorageFolder::GetFolderFromPathAsync(outputPath).get();
    StorageFile outimagefile = currentfolder.CreateFileAsync(outputDataImageFileName, CreationCollisionOption::ReplaceExisting).get();
    IRandomAccessStream writestream = outimagefile.OpenAsync(FileAccessMode::ReadWrite).get();
    BitmapEncoder encoder = BitmapEncoder::CreateAsync(BitmapEncoder::JpegEncoderId(), writestream).get();
    encoder.SetSoftwareBitmap(resultTensor.VideoFrame().SoftwareBitmap());
    encoder.FlushAsync().get();
    printf("%ws is saved to disk\n", outputDataImageFileName.c_str());
}