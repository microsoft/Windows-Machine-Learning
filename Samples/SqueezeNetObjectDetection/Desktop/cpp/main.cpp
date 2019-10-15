// main.cpp : Defines the entry point for the console application.
//

#include "pch.h"

using namespace winrt;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::AI::MachineLearning;
using namespace Windows::Media;
using namespace Windows::Graphics::Imaging;
using namespace Windows::Storage;
using namespace std;

// globals
vector<string> labels;
string labelsFileName("labels.txt");
LearningModelDeviceKind deviceKind = LearningModelDeviceKind::Default;
string deviceName = "default";
hstring imagePath;

// helper functions
string GetModulePath();
void LoadLabels();
VideoFrame LoadImageFile(hstring filePath, ColorManagementMode colorManagementMode);
void PrintResults(IVectorView<float> results);
bool ParseArgs(int argc, char* argv[]);
ColorManagementMode GetColorManagementMode(const LearningModel& model);

wstring GetModelPath()
{
    wostringstream woss;
    woss << GetModulePath().c_str();
    woss << "SqueezeNet.onnx";
    return woss.str();
}

// MAIN !
// usage: SqueezeNet [imagefile] [cpu|directx]
int main(int argc, char* argv[])
{
    init_apartment();

    // did they pass in the args 
    if (ParseArgs(argc, argv) == false)
    {
        printf("Usage: %s [imagefile] [cpu|directx]", argv[0]);
        return -1;
    }

    // Get model path
    auto modelPath = GetModelPath();

    // load the model
    printf("Loading modelfile '%ws' on the '%s' device\n", modelPath.c_str(), deviceName.c_str());
    DWORD ticks = GetTickCount();
    auto model = LearningModel::LoadFromFilePath(modelPath);
    ticks = GetTickCount() - ticks;
    printf("model file loaded in %d ticks\n", ticks);

    // get model color management mode
    printf("Getting model color management mode...\n");
    ColorManagementMode colorManagementMode = GetColorManagementMode(model);

    // load the image
    printf("Loading the image...\n");
    auto imageFrame = LoadImageFile(imagePath, colorManagementMode);
    // now create a session and binding
    LearningModelSession session(model, LearningModelDevice(deviceKind));
    LearningModelBinding binding(session);

    // bind the intput image
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

bool ParseArgs(int argc, char* argv[])
{
    if (argc < 2)
    {
        return false;
    }
    // get the image file
    imagePath = hstring(wstring_to_utf8().from_bytes(argv[1]));
    // did they pass a third arg?
    if (argc >= 3)
    {
        deviceName = argv[2];
        if (deviceName == "cpu")
        {
            deviceKind = LearningModelDeviceKind::Cpu;
        }
        else if (deviceName == "directx")
        {
            deviceKind = LearningModelDeviceKind::DirectX;
        }
        else
        {
            deviceName = "default";
            deviceKind = LearningModelDeviceKind::Default;
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
        printf("failed to load the %s file.  Make sure it exists in the same folder as the app\r\n", labelsFileName.c_str());
        exit(EXIT_FAILURE);
    }

    std::string s;
    while (std::getline(labelFile, s, ','))
    {
        int labelValue = atoi(s.c_str());
        if (static_cast<uint32_t>(labelValue) >= labels.size())
        {
            labels.resize(labelValue + 1);
        }
        std::getline(labelFile, s);
        labels[labelValue] = s;
    }
}

ColorManagementMode GetColorManagementMode(const LearningModel& model)
{
    // Get model color space gamma
    hstring gammaSpace = L"";
    try
    {
        gammaSpace = model.Metadata().Lookup(L"Image.ColorSpaceGamma");
    }
    catch (...)
    {
        printf("    Model does not have color space gamma information. Will color manage to sRGB by default...\n");
    }
    if (gammaSpace == L"" || _wcsicmp(gammaSpace.c_str(), L"SRGB") == 0)
    {
        return ColorManagementMode::ColorManageToSRgb;
    }
    // Due diligence should be done to make sure that the input image is within the model's colorspace. There are multiple non-sRGB color spaces.
    printf("    Model metadata indicates that color gamma space is : %ws. Will not manage color space...\n", gammaSpace.c_str());
    return ColorManagementMode::DoNotColorManage;
}

VideoFrame LoadImageFile(hstring filePath, ColorManagementMode colorManagementMode)
{
    try
    {
        // open the file
        StorageFile file = StorageFile::GetFileFromPathAsync(filePath).get();
        // get a stream on it
        auto stream = file.OpenAsync(FileAccessMode::Read).get();
        // Create the decoder from the stream
        BitmapDecoder decoder = BitmapDecoder::CreateAsync(stream).get();

        SoftwareBitmap softwareBitmap = NULL;
        try
        {
            softwareBitmap = decoder.GetSoftwareBitmapAsync(
                decoder.BitmapPixelFormat(),
                decoder.BitmapAlphaMode(),
                BitmapTransform(),
                ExifOrientationMode::RespectExifOrientation,
                colorManagementMode
            ).get();
        }
        catch (hresult_error hr)
        {
            printf("    Failed to create SoftwareBitmap! Please make sure that input image is within the model's colorspace.\n");
            printf("    %ws\n", hr.message().c_str());
            exit(hr.code());
        }

        // load a videoframe from it
        VideoFrame inputImage = VideoFrame::CreateWithSoftwareBitmap(softwareBitmap);
        // all done
        return inputImage;
    }
    catch (...)
    {
        printf("    Failed to load the image file, make sure you are using fully qualified paths\r\n");
        exit(EXIT_FAILURE);
    }
}

void PrintResults(IVectorView<float> results)
{
    // load the labels
    LoadLabels();

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
