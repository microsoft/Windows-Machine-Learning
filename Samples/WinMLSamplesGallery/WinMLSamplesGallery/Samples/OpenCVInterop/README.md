# WinML Samples Gallery: OpenCV Interop
This sample demonstrates how to interop between [Windows ML](https://docs.microsoft.com/en-us/windows/ai/windows-ml/) and [OpenCV](https://github.com/opencv/opencv).

OpenCV (Open Source Computer Vision Library) is an open source computer vision and machine learning software library. OpenCV was built to provide a common infrastructure for computer vision applications and to accelerate the use of machine perception in the commercial products. 

The demo will run [SqueezeNet](https://github.com/onnx/models/tree/master/vision/classification/squeezenet) image classification in WindowsML and consume images loaded and preprocessed using OpenCV.

OpenCV will be used to load images, add salt and pepper noise to the base image, and denoise the image using media blur.
Windows ML will be used to resize and tensorize the image into NCHW format, as well as perform image classification.


<img src="docs/screenshot.png" width="650"/>

- [Licenses](#licenses)
- [Getting Started](#getting-started)
- [Feedback]($feedback)
- [External Links](#links)


## Licenses

- [OpenCV LICENSE](../../../../../external/opencv/4.5.4/LICENSE)

## Getting Started
- Check out the [source](https://github.com/microsoft/Windows-Machine-Learning/blob/a08bb78dd3cd9a6449e2d02ae3cbb41b10ead463/Samples/WinMLSamplesGallery/WinMLSamplesGallery/Samples/ImageClassifier/ImageClassifier.xaml.cs#L123)

## Feedback
Please file an issue [here](https://github.com/microsoft/Windows-Machine-Learning/issues/new) if you encounter any issues with this sample.

## External Links

- [Windows ML Library (WinML)](https://docs.microsoft.com/en-us/windows/ai/windows-ml/)
- [DirectML](https://github.com/microsoft/directml)
- [ONNX Model Zoo](https://github.com/onnx/models)
- [Windows UI Library (WinUI)](https://docs.microsoft.com/en-us/windows/apps/winui/) 