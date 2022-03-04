#include "pch.h"
#include "PixRedistMiddleware.h"
#include "WinMLHelper.h"

void LoadModel()
{
    // load the model
    printf("Loading modelfile '%ws' on the '%s' device\n", modelPath.c_str(), deviceName.c_str());
    DWORD ticks = GetTickCount();
    model = LearningModel::LoadFromFilePath(modelPath);
    ticks = GetTickCount() - ticks;
    printf("model file loaded in %d ticks\n", ticks);
}

VideoFrame LoadImageFile(hstring filePath)
{
    printf("Loading the image...\n");
    DWORD ticks = GetTickCount();
    VideoFrame inputImage = nullptr;

    TCHAR buffer[MAX_PATH] = { 0 };
    wstring dir;
    GetCurrentDirectory(MAX_PATH, buffer);
    dir.assign(buffer).append(filePath);

    try
    {
        // open the file
        StorageFile file = StorageFile::GetFileFromPathAsync(dir).get();
        // get a stream on it
        auto stream = file.OpenAsync(FileAccessMode::Read).get();
        // Create the decoder from the stream
        BitmapDecoder decoder = BitmapDecoder::CreateAsync(stream).get();
        // get the bitmap
        SoftwareBitmap softwareBitmap = decoder.GetSoftwareBitmapAsync().get();
        // load a videoframe from it
        imageFrame = VideoFrame::CreateWithSoftwareBitmap(softwareBitmap);
    }
    catch (...)
    {
        printf("failed to load the image file, make sure you are using fully qualified paths\r\n");
        exit(EXIT_FAILURE);
    }

    ticks = GetTickCount() - ticks;
    printf("image file loaded in %d ticks\n", ticks);
    // all done
    return imageFrame;
}

void CreateSession(ID3D12CommandQueue* commandQueue)
{
    printf("Binding the model...\n");

    winrt::com_ptr<::IUnknown> spUnk;
    auto factory = winrt::get_activation_factory<LearningModelDevice, ILearningModelDeviceFactoryNative>();
    factory->CreateFromD3D12CommandQueue(commandQueue, spUnk.put());
    LearningModelDevice dmlDeviceCustom = spUnk.as<LearningModelDevice>();

    // now create a session
    session = LearningModelSession{ model, dmlDeviceCustom };
}

void BindModel()
{
    printf("Binding the model...\n");
    DWORD ticks = GetTickCount();
    binding = LearningModelBinding{ session };
    // bind the intput image
    binding.Bind(L"data_0", ImageFeatureValue::CreateFromVideoFrame(imageFrame));
    // bind the output
    vector<int64_t> shape({ 1, 1000, 1, 1 });
    binding.Bind(L"softmaxout_1", TensorFloat::Create(shape));

    ticks = GetTickCount() - ticks;
    printf("Model bound in %d ticks\n", ticks);
}

void EvaluateModel()
{
    // now run the model
    printf("Running the model...\n");
    DWORD ticks = GetTickCount();

    auto results = session.Evaluate(binding, L"RunId");

    ticks = GetTickCount() - ticks;
    printf("model run took %d ticks\n", ticks);

    // get the output
    auto resultTensor = results.Outputs().Lookup(L"softmaxout_1").as<TensorFloat>();
    auto resultVector = resultTensor.GetAsVectorView();
    PrintResults(resultVector);
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

void LoadLabels()
{
    // Parse labels from labels file.  We know the file's entries are already sorted in order.
    ifstream labelFile{ labelsFilePath, ifstream::in };
    if (labelFile.fail())
    {
        printf("failed to load the %s file.  Make sure it exists in the same folder as the app\r\n", labelsFilePath.c_str());
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

