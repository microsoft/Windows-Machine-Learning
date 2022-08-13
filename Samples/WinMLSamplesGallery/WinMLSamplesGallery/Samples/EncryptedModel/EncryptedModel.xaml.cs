using Microsoft.AI.MachineLearning;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Media;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Windows.Foundation.Metadata;
using Windows.Graphics.Imaging;
using Windows.Media;
using Windows.UI;
using WinMLSamplesGallery.Common;
using WinMLSamplesGallery.Controls;
using WinMLSamplesGalleryNative;

namespace WinMLSamplesGallery.Samples
{
    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class EncryptedModel : Page
    {
#pragma warning disable CA1416 // Validate platform compatibility
        public bool initialized_ = false;

        public EncryptedModel()
        {
            this.InitializeComponent();
            initialized_ = true;

            // Initialize the sample with the decrypted model
            DecryptAndEvauluate();
        }


        private void OnDecryptClick(object sender, RoutedEventArgs e)
        {
            DecryptAndEvauluate();
        }

        private void DecryptAndEvauluate()
        {
            // Load the encrypted model.
            // The encrypted model (encrypted.onnx) is embedded as a resource in
            // the native binary: WinMLSamplesGalleryNative.dll.
            var inferenceModel = WinMLSamplesGalleryNative.EncryptedModels.LoadEncryptedResource(DecryptionKey.Password);
            var postProcessingModel = TensorizationModels.SoftMaxThenTopK(10);

            // Update the status
            var isModelDecrypted = inferenceModel != null;
            UpdateStatus(isModelDecrypted);

            // If loading the decrypted model failed (ie: due to an invalid key/password),
            // then skip performing evaluate.
            if (!isModelDecrypted)
            {
                return;
            }

            // Draw the image to classify in the Image control
            var decoder = ImageHelper.CreateBitmapDecoderFromPath("ms-appx:///InputData/hummingbird.jpg");

            // Create sessions
            var device = new LearningModelDevice(LearningModelDeviceKind.Cpu);
            var options = new LearningModelSessionOptions()
            {
                CloseModelOnSessionCreation = true // Close the model to prevent extra memory usage
            };
            var inferenceSession = new LearningModelSession(inferenceModel, device, options);
            var postProcessingSession = new LearningModelSession(postProcessingModel, device, options);

            // Classify the current image
            var softwareBitmap = decoder.GetSoftwareBitmapAsync().GetAwaiter().GetResult();
            var input = VideoFrame.CreateWithSoftwareBitmap(softwareBitmap);

            // Inference
            var inferenceResults = Evaluate(inferenceSession, input);
            var inferenceOutput = inferenceResults.Outputs.First().Value;

            // PostProcess
            var postProcessedOutputs = Evaluate(postProcessingSession, inferenceOutput);
            var topKValues = (TensorFloat)postProcessedOutputs.Outputs["TopKValues"];
            var topKIndices = (TensorInt64Bit)postProcessedOutputs.Outputs["TopKIndices"];

            // Return results
            var probabilities = topKValues.GetAsVectorView();
            var indices = topKIndices.GetAsVectorView();
            var labels = indices.Select((index) => ClassificationLabels.ImageNet[index]);

            // Render the classification and probabilities
            RenderInferenceResults(labels, probabilities);
        }

        private void UpdateStatus(bool isModelDecrypted)
        {
            Fail.Visibility = isModelDecrypted ? Visibility.Collapsed : Visibility.Visible;
            Succeed.Visibility = isModelDecrypted ? Visibility.Visible : Visibility.Collapsed;

            if (isModelDecrypted == false) {
                InferenceResults.ItemsSource = null;
                InferenceResults.SelectedIndex = 0;
            }
        }

        private void RenderInferenceResults(IEnumerable<string> labels, IReadOnlyList<float> probabilities)
        {
            var indices = Enumerable.Range(1, probabilities.Count);
            var zippedResults = indices.Zip(labels.Zip(probabilities));
            var results = zippedResults.Select(
                (zippedResult) =>
                    new Controls.Prediction
                    {
                        Index = zippedResult.First,
                        Name = zippedResult.Second.First.Trim(new char[] { ',' }),
                        Probability = zippedResult.Second.Second.ToString("P")
                    });
            InferenceResults.ItemsSource = results;
            InferenceResults.SelectedIndex = 0;
        }

        private static LearningModelEvaluationResult Evaluate(LearningModelSession session, object input)
        {
            // Create the binding
            var binding = new LearningModelBinding(session);

            // Create an emoty output, that will keep the output resources on the GPU
            // It will be chained into a the post processing on the GPU as well
            var output = TensorFloat.Create();

            // Bind inputs and outputs
            // For squeezenet these evaluate to "data", and "squeezenet0_flatten0_reshape0"
            string inputName = session.Model.InputFeatures[0].Name;
            string outputName = session.Model.OutputFeatures[0].Name;
            binding.Bind(inputName, input);

            var outputBindProperties = new PropertySet();
            outputBindProperties.Add("DisableTensorCpuSync", PropertyValue.CreateBoolean(true));
            binding.Bind(outputName, output, outputBindProperties);

            // Evaluate
            return session.Evaluate(binding, "");
        }
#pragma warning restore CA1416 // Validate platform compatibility

    }
}
