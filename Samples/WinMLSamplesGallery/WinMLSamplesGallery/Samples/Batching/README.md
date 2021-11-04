# WinML Samples Gallery: Batching
 Perform infererence over multiple inputs at once to increase runtime performance.
 
 Use this sample to compare inference runtime performace with and without batched inputs. 50 images will be passed through SqueezeNet for classification. Set a batch size and click Start Inference to compare.
 
<img src="docs/BatchingScreenshot.png" width="650"/>

- [Getting Started](#getting-started)
- [BatchSizeOverride](#batchsizeoverride)
- [Feedback](#feedback)
- [External Links](#external-links)

## Getting Started
- Check out the [source](https://github.com/microsoft/Windows-Machine-Learning/blob/master/Samples/WinMLSamplesGallery/WinMLSamplesGallery/Samples/Batching/Batching.xaml.cs).
- Learn how to [add the BatchSizeOverride property](https://github.com/microsoft/Windows-Machine-Learning/blob/master/Samples/WinMLSamplesGallery/WinMLSamplesGallery/Samples/Batching/Batching.xaml.cs#L125) to your session.
- Learn how to [evaluate batched input](https://github.com/microsoft/Windows-Machine-Learning/blob/master/Samples/WinMLSamplesGallery/WinMLSamplesGallery/Samples/Batching/Batching.xaml.cs#L186).

## BatchSizeOverride
- Learn more about the BatchSizeOverride property from the [docs](https://docs.microsoft.com/en-us/uwp/api/windows.ai.machinelearning.learningmodelsessionoptions.batchsizeoverride?view=winrt-22000).
- **Note**: To use batched inputs, the model must have a value of -1 or a variable name in the input batch dimension. Additionally, the DATA_BATCH Denotation must be added on the batch dimension. To edit your model you can use tools like [WinML Dashboard](https://github.com/Microsoft/Windows-Machine-Learning/tree/master/Tools/WinMLDashboard).

## Feedback
Please file an issue [here](https://github.com/microsoft/Windows-Machine-Learning/issues/new) if you encounter any issues with the WinML Samples Gallery or wish to request a new sample.

## External Links

- [Windows ML Library (WinML)](https://docs.microsoft.com/en-us/windows/ai/windows-ml/)
- [DirectML](https://github.com/microsoft/directml)
- [ONNX Model Zoo](https://github.com/onnx/models)
- [Windows UI Library (WinUI)](https://docs.microsoft.com/en-us/windows/apps/winui/) 
