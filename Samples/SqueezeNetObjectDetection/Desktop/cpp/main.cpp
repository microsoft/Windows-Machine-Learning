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
VideoFrame LoadImageFile(hstring filePath);
void PrintResults(IVectorView<float> results);
bool ParseArgs(int argc, char* argv[]);

wstring GetModelPath()
{
    wostringstream woss;
    woss << GetModulePath().c_str();
    woss << "SqueezeNet.onnx";
    return woss.str();
}

// MAIN !
// usage: SqueezeNet [modelfile] [imagefile] [cpu|directx]
int main(int argc, char* argv[])
{
    init_apartment();

    // did they pass in the args 
    if (ParseArgs(argc, argv) == false)
    {
        printf("Usage: %s [modelfile] [imagefile] [cpu|directx]", argv[0]);
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

    // load the image
    printf("Loading the image...\n");
    auto imageFrame = LoadImageFile(imagePath);
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
    // did they pass a fourth arg?
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
