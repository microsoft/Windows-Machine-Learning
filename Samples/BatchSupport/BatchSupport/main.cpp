#include "pch.h"
#include "SampleHelper.h"

using namespace winrt;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::AI::MachineLearning;
using namespace Windows::Media;
using namespace Windows::Graphics::Imaging;
using namespace Windows::Storage;
using namespace winrt::Windows::Storage::Streams;
using namespace std;

#define BATCH_SIZE 3
hstring executionPath = static_cast<hstring>(SampleHelper::GetModulePath().c_str());
string modelType = "freeBatchSize";
bool ParseArgs(int argc, char* argv[]);

int main(int argc, char* argv[])
{
    init_apartment();

    // did they pass in the args 
    if (ParseArgs(argc, argv) == false)
    {
      printf("Usage: %s [fixedBatchSize|freeBatchSize]", argv[0]);
    }

    // Get model path and image path
    hstring modelPath;
    if (modelType == "fixedBatchSize") {
      modelPath = executionPath + L"SqueezeNet_free.onnx";
    }
    else {
      modelPath = executionPath + L"SqueezeNet.onnx";
    }
    auto imagePath = executionPath + L"kitten_224.png";

    // load the model
    printf("Loading modelfile '%ws' on the CPU\n", modelPath.c_str());
    DWORD ticks = GetTickCount();
    auto model = LearningModel::LoadFromFilePath(modelPath);
    ticks = GetTickCount() - ticks;
    printf("model file loaded in %d ticks\n", ticks);

    // load the image
    printf("Loading the image...\n");
    auto imageFrame = SampleHelper::LoadImageFile(imagePath);
    
    // Create input Tensorfloats with 3 copied tensors.
    std::vector<float> imageVector = SampleHelper::SoftwareBitmapToSoftwareTensor(imageFrame.SoftwareBitmap());
    std::vector<float> inputVector = imageVector;
    inputVector.insert(inputVector.end(), imageVector.begin(), imageVector.end());
    inputVector.insert(inputVector.end(), imageVector.begin(), imageVector.end());

    auto inputShape = std::vector<int64_t>{ BATCH_SIZE, 3, 224, 224 };
    auto inputValue = 
      TensorFloat::CreateFromIterable(
        inputShape,
        single_threaded_vector<float>(std::move(inputVector)).GetView());

    // Create input VideoFrames with 3 copied images
    vector<VideoFrame> inputFrames = {};
    for (uint32_t i = 0; i < BATCH_SIZE; ++i) {
      inputFrames.emplace_back(imageFrame);
    }
    auto videoFrames = winrt::single_threaded_vector(move(inputFrames));

    // now create a session and binding
    LearningModelDeviceKind deviceKind = LearningModelDeviceKind::Cpu;

    LearningModelSessionOptions options;
    if ("freeBatchSize" == modelType) {
      options.BatchSizeOverride(static_cast<uint32_t>(BATCH_SIZE));
    }
    LearningModelSession session(model, LearningModelDevice(deviceKind), options);
    LearningModelBinding binding(session);

    // bind the intput image
    printf("Binding...\n");
    auto inputFeatureDescriptor = model.InputFeatures().First();
    binding.Bind(inputFeatureDescriptor.Current().Name(), videoFrames);

    // bind output tensor
    auto outputShape = std::vector<int64_t>{ BATCH_SIZE, 1000, 1, 1};
    auto outputValue = TensorFloat::Create(outputShape);
    std::wstring outputDataBindingName = std::wstring(model.OutputFeatures().First().Current().Name());
    binding.Bind(outputDataBindingName, outputValue);

    
    // bind output videoFrames
    // now run the model
    printf("Running the model...\n");
    ticks = GetTickCount();
    auto results = session.EvaluateAsync(binding, L"RunId").get();
    ticks = GetTickCount() - ticks;
    printf("model run took %d ticks\n", ticks);

    SampleHelper::PrintResults(outputValue.GetAsVectorView());

}

bool ParseArgs(int argc, char* argv[])
{
  if (argc < 2)
  {
    return false;
  }
  modelType = argv[1];
  return true;
}
