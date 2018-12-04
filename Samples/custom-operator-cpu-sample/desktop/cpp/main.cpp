#include "pch.h"

#include "operators/customoperatorprovider.h"

using namespace winrt;
using namespace Windows::Foundation;
using namespace Windows::AI::MachineLearning;

struct CommandLineInterpreter
{
    std::vector<std::wstring> m_commandLineArgs;

    CommandLineInterpreter(int argc, wchar_t * argv[]) :
        m_commandLineArgs(&argv[0], &argv[0] + argc)
    {}

    static std::wstring GetModelPath(const wchar_t* pName)
    {
        wchar_t wzModuleFilePath[MAX_PATH + 1];
        GetModuleFileName(NULL, wzModuleFilePath, MAX_PATH + 1);
        std::wstring moduleFilePath(wzModuleFilePath);

        auto exePath =
            std::wstring(
                moduleFilePath.begin(),
                moduleFilePath.begin() + moduleFilePath.find_last_of(L"\\"));

        return exePath + L"\\" = pName;
    }

    std::wstring TryGetModelPath()
    {
        if (m_commandLineArgs.size() == 2)
        {
            if (0 == _wcsicmp(L"debug", m_commandLineArgs[1].c_str()))
            {
                return GetModelPath(L"squeezenet.onnx");
            }
            else if (0 == _wcsicmp(L"relu", m_commandLineArgs[1].c_str()))
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
};


int wmain(int argc, wchar_t * argv[])
{
    init_apartment();

    CommandLineInterpreter args(argc, argv);

    auto modelPath = args.TryGetModelPath();
    if (modelPath.empty())
    {
        wprintf(L"This executable only accepts a single parameter with value: debug, relu, or noisyrelu.");
        return 0;
    }

    using Model = LearningModel;
    using Binding = LearningModelBinding;
    using Device = LearningModelDevice;
    using Session = LearningModelSession;
    using Kind = LearningModelDeviceKind;

    printf("Creating the custom operator provider.\n");
    auto customOperatorProvider = winrt::make<CustomOperatorProvider>();
    auto provider = customOperatorProvider.as<ILearningModelOperatorProvider>();

    // load the model with the custom operator provider
    printf("Calling LoadFromFilePath('%ws').\n", modelPath.c_str());
    auto model = Model::LoadFromFilePath(modelPath, provider);

    printf("Creating ModelSession.\n");
    Session session(model, Device(Kind::Default));

    printf("Create the ModelBinding binding collection.\n");
    Binding bindings(session);

    printf("Create the input tensor.\n");
    auto inputShape = std::vector<int64_t>{ 5 };
    auto inputData = std::vector<float>{ -50.f, -25.f, 0.f, 25.f, 50.f };
    auto inputValue =
        TensorFloat::CreateFromIterable(
            inputShape,
            single_threaded_vector<float>(std::move(inputData)).GetView());

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

    return 0;
}
