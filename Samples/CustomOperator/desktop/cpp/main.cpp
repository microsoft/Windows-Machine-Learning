#include "pch.h"
#include "FileHelper.h"
#include "operators/customoperatorprovider.h"

using namespace winrt;
using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Foundation::Collections;
using namespace winrt::Windows::Media;
using namespace winrt::Windows::Graphics::Imaging;
using namespace winrt::Windows::Storage;
using namespace winrt::Windows::AI::MachineLearning;
using namespace std;

struct CommandLineInterpreter
{
    vector<wstring> m_commandLineArgs;

    CommandLineInterpreter(int argc, wchar_t * argv[]) :
        m_commandLineArgs(&argv[0], &argv[0] + argc)
    {}

    static wstring GetModelPath(const wchar_t* pName)
    {
        wchar_t wzModuleFilePath[MAX_PATH + 1];
        GetModuleFileName(NULL, wzModuleFilePath, MAX_PATH + 1);
        return FileHelper::GetModulePath() + L"\\" + pName;
    }

    wstring TryGetModelPath()
    {
        if (m_commandLineArgs.size() >= 2)
        {
            if (0 == _wcsicmp(L"debug", m_commandLineArgs[1].c_str()))
            {
                return GetModelPath(L"squeezenet_debug_one_output.onnx");
            }
            else if (0 == _wcsicmp(L"relu", m_commandLineArgs[1].c_str()) || 0 == _wcsicmp(L"relu_gpu", m_commandLineArgs[1].c_str()))
            {
                return GetModelPath(L"relu.onnx");
            }
            else if (0 == _wcsicmp(L"noisyrelu", m_commandLineArgs[1].c_str()))
            {
                return GetModelPath(L"noisy_relu.onnx");
            }
        }

        return L"";
    }

    bool UseGpu()
    {
        if (m_commandLineArgs.size() >= 2)
        {
            if (0 == _wcsicmp(L"relu_gpu", m_commandLineArgs[1].c_str()))
            {
                return true;
            }
        }

        return false;
    }

    bool IsDebugOperator() {
        return 0 == _wcsicmp(L"debug", m_commandLineArgs[1].c_str());
    }

    hstring GetImagePath() {
        if (m_commandLineArgs.size() == 3) {
            return hstring(m_commandLineArgs[2]);
        }
        else {
            return L"";
        }
    }
};

using Model = LearningModel;
using Binding = LearningModelBinding;
using Device = LearningModelDevice;
using Session = LearningModelSession;
using Kind = LearningModelDeviceKind;
vector<string> labels;

void PrintResults(IVectorView<float> results)
{
    // load the labels
    auto modulePath = FileHelper::GetModulePath();
    std::string labelsFilePath =
      std::string(modulePath.begin(), modulePath.end()) + "Labels.txt";
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

void RunSqueezeNet(Session session, Model model, hstring imagePath) {
    LearningModelBinding bindings(session);

    // load the image
    printf("Loading the image...\n");
    auto imageFrame = FileHelper::LoadImageFile(imagePath);

    // bind the input image
    printf("Binding...\n");
    ImageFeatureValue i = ImageFeatureValue::CreateFromVideoFrame(imageFrame);
    bindings.Bind(model.InputFeatures().GetAt(0).Name(), i);
    // temp: bind the output (we don't support unbound outputs yet)
    vector<int64_t> shape({ 1, 1000, 1, 1 });
    bindings.Bind(model.OutputFeatures().GetAt(0).Name(), TensorFloat::Create(shape));

    // now run the model
    printf("Running the model...intermediate debug operators will be output to specified file paths\n");
    DWORD ticks = GetTickCount();
    ticks = GetTickCount();
    auto results = session.Evaluate(bindings, L"RunId");
    ticks = GetTickCount() - ticks;
    printf("model run took %d ticks\n", ticks);

    // get the output
    auto resultTensor = results.Outputs().Lookup(L"softmaxout_1").as<TensorFloat>();
    auto resultVector = resultTensor.GetAsVectorView();
    PrintResults(resultVector);
}


void RunRelu(Session session)
{
    printf("Create the ModelBinding binding collection.\n");
    Binding bindings(session);

    printf("Create the input tensor.\n");
    auto inputShape = vector<int64_t>{ 5 };
    auto inputData = vector<float>{ -50.f, -25.f, 0.f, 25.f, 50.f };
    auto inputValue =
        TensorFloat::CreateFromIterable(
            inputShape,
            single_threaded_vector<float>(move(inputData)).GetView());

    printf("Binding input tensor to the ModelBinding binding collection.\n");
    bindings.Bind(L"X", inputValue);

    printf("Create the output tensor.\n");
    auto outputValue = TensorFloat::Create();

    printf("Binding output tensor to the ModelBinding binding collection.\n");
    bindings.Bind(L"Y", outputValue);

    printf("Calling EvaluateSync()\n");
    hstring correlationId;
    session.Evaluate(bindings, correlationId);

    printf("Getting output binding (Y), featureKind=%d, dataKind=%d, dims=%d\n",
        outputValue.Kind(),
        outputValue.TensorKind(),
        outputValue.Shape().Size());
    auto buffer = outputValue.GetAsVectorView();

    printf("Got output binding data, size=(%d).\n", buffer.Size());
    for (auto resultItem : buffer)
    {
        printf("%f\n", resultItem);
    }
    printf("Done\n");
}


int wmain(int argc, wchar_t * argv[])
{
    init_apartment();

    CommandLineInterpreter args(argc, argv);

    auto modelPath = args.TryGetModelPath();
    if (modelPath.empty())
    {
        wprintf(L"Must supply first parameter: <debug|relu|relu_gpu|noisyrelu>");
        return 0;
    }

    printf("Creating the custom operator provider.\n");
    auto customOperatorProvider = winrt::make<CustomOperatorProvider>();
    auto provider = customOperatorProvider.as<ILearningModelOperatorProvider>();

    // load the model with the custom operator provider
    printf("Calling LoadFromFilePath('%ws').\n", modelPath.c_str());
    auto model = Model::LoadFromFilePath(modelPath, provider);

    printf("Creating ModelSession.\n");
    Session session(model, Device(args.UseGpu() ? Kind::DirectXHighPerformance : Kind::Default));

    if (args.IsDebugOperator()) {
        // debug operator sample model is squeezenet as it is best demonstrated on an image recognition model
        hstring imagePath = args.GetImagePath();

        if (imagePath.empty()) {
            wprintf(L"Must supply path to image file for debug run");
            return 0;
        }

        RunSqueezeNet(session, model, imagePath);
    }
    else {
        RunRelu(session);
    }
    return 0;
}
