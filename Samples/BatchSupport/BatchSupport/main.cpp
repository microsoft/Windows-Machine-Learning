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
string modelType = "freeBatchSize";
string inputType = "TensorFloat";

hstring executionPath =
    static_cast<hstring>(SampleHelper::GetModulePath().c_str());

bool ParseArgs(int argc, char *argv[]);

int main(int argc, char *argv[]) {
  init_apartment();

  // did they pass in the args
  if (ParseArgs(argc, argv) == false) {
    printf("Usage: %s [freeBatchSize|fixedBatchSize] [TensorFloat|VideoFrame] \n", argv[0]);
  }

  // load the model
  hstring modelPath = SampleHelper::GetModelPath(modelType);
  printf("Loading modelfile '%ws' on the CPU\n", modelPath.c_str());
  auto model = LearningModel::LoadFromFilePath(modelPath);

  // now create a session and binding
  LearningModelDeviceKind deviceKind = LearningModelDeviceKind::Cpu;
  LearningModelSession session = nullptr;
  if ("freeBatchSize" == modelType) { 
    // If the model has free dimensional batch, override the free dimension with batch_size 
    // for performance improvement.
    LearningModelSessionOptions options;
    printf("Override Batch Size by %d\n", BATCH_SIZE);
    options.BatchSizeOverride(static_cast<uint32_t>(BATCH_SIZE));
    session = LearningModelSession(model, LearningModelDevice(deviceKind), options);
  }
  else {
    session = LearningModelSession(model, LearningModelDevice(deviceKind));
  }
  LearningModelBinding binding(session);

  // bind the intput image
  printf("Binding...\n");
  auto inputFeatureDescriptor = model.InputFeatures().First();

  if (inputType == "TensorFloat") { // if bind TensorFloat
    // Create input Tensorfloats with 3 copied tensors.
    TensorFloat inputTensorValue = SampleHelper::CreateInputTensorFloat();
    binding.Bind(inputFeatureDescriptor.Current().Name(), inputTensorValue);
  } else { // else bind VideoFrames
    // Create input VideoFrames with 3 copied images
    auto inputVideoFrames = SampleHelper::CreateVideoFrames();
    binding.Bind(inputFeatureDescriptor.Current().Name(), inputVideoFrames);
  }

  // now run the model
  printf("Running the model...\n");
  DWORD ticks = GetTickCount();
  auto results = session.EvaluateAsync(binding, L"RunId").get();
  ticks = GetTickCount() - ticks;
  printf("model run took %d ticks\n", ticks);

  // Print Results
  auto outputs = results.Outputs().Lookup(L"softmaxout_1").as<TensorFloat>();
  auto outputShape = outputs.Shape();
  printf("output dimensions [%d, %d, %d, %d]\n", outputShape.GetAt(0), outputShape.GetAt(1), outputShape.GetAt(2), outputShape.GetAt(3));
  SampleHelper::PrintResults(outputs.GetAsVectorView());
}

bool ParseArgs(int argc, char *argv[]) {
  if (argc < 2) {
    return false;
  }
  modelType = argv[1];
  if (argc > 3) {
    inputType = argv[2];
  }
  return true;
}
