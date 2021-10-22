# Windows Machine Learning

Windows Machine Learning is a high-performance machine learning inference API that is powered by [ONNX Runtime](https://onnxruntime.ai/) and [DirectML](https://docs.microsoft.com/en-us/windows/ai/directml/dml).


[![Alt text](https://i.ytimg.com/vi/8MCDSlm326U/hq720.jpg?sqp=-oaymwEcCOgCEMoBSFXyq4qpAw4IARUAAIhCGAFwAcABBg==&rs=AOn4CLCFThAhMuWU3UrHtPjc4Ttz9SSkpQ)](https://www.youtube.com/watch?v=8MCDSlm326U)

The Windows ML API is a [Windows Runtime Component](https://docs.microsoft.com/en-us/windows/uwp/winrt-components/) and is suitable for high-performance, low-latency applications such as frameworks, games, and other real-time applications as well as applications built with high-level languages.


This repo contains Windows Machine Learning samples and tools that demonstrate how to build machine learning powered scenarios into Windows applications.

- [Getting Started with Windows ML](#getting-started-with-windows-ml)
- [Model Samples](#model-samples)
- [Advanced Scenario Samples](#advanced-scenario-samples)
- [Developer Tools](#developer-tools)
- [Feedback](#feedback)
- [External Links](#external-links)
- [Contributing](#contributing)

For additional information on Windows ML, including step-by-step tutorials and how-to guides, please visit the [Windows ML documentation](https://docs.microsoft.com/en-us/windows/ai/).


| Sample/Tool | Status |
|---------|--------------|
| All Samples | [![Build Status](https://microsoft.visualstudio.com/WindowsAI/_apis/build/status/WinML/Samples/Github%20Public%20Samples%20Build%20RS5?branchName=master)](https://microsoft.visualstudio.com/WindowsAI/_build/latest?definitionId=39302&branchName=master) |
| WinmlRunner | [![Build Status](https://microsoft.visualstudio.com/WindowsAI/_apis/build/status/WInMLRunner%20CI%20Build?branchName=master)](https://microsoft.visualstudio.com/WindowsAI/_build/latest?definitionId=38654&branchName=master) |
| WinML Dashboard | [![Build Status](https://microsoft.visualstudio.com/WindowsAI/_apis/build/status/WinML/WinMLTools/WinML%20Dashboard%20CI%20Build%20(Github)?branchName=master)](https://microsoft.visualstudio.com/WindowsAI/_build/latest?definitionId=39375&branchName=master) |

## Getting Started with Windows ML

**Prerequisites**
- [Visual Studio 2017 Version 15.7.4 or Newer](https://developer.microsoft.com/en-us/windows/downloads)


Windows ML offers machine learning inferencing via the inbox Windows SDK as well as a redistributable NuGet package. The table below highlights the availability, distribution, language support, servicing, and forward compatibility aspects of the In-Box and NuGet package for Windows ML.

| |In-Box|	NuGet|
|-|-----|------|
|Availability|	[Windows 10 - Build 17763 (RS5) or Newer](https://www.microsoft.com/en-us/software-download/windows10)<br/>For more detailed information about version support, checkout our [docs](https://docs.microsoft.com/en-us/uwp/api/windows.ai.machinelearning?view=winrt-22000).	| [Windows 8.1 or Newer](https://www.microsoft.com/en-us/software-download/windows8ISO)<br/>**NOTE**: Some APIs (ie: VideoFrame) are not available on older OSes.|
|Windows SDK|	[Windows SDK - Build 17763 (RS5) or Newer](https://developer.microsoft.com/en-us/windows/downloads/sdk-archive/) |	[Windows SDK - Build 17763 (RS5) or Newer](https://developer.microsoft.com/en-us/windows/downloads/sdk-archive/)
|Distribution|	Built into Windows |	Package and distribute as part of your application
|Servicing|	Microsoft-driven (customers benefit automatically)	| Developer-driven
|Forward| compatibility	Automatically rolls forward with new features	| Developer needs to update package manually

Learn more [here](https://docs.microsoft.com/en-us/windows/ai/windows-ml/get-started).

## Model Samples
In this section you will find various model samples for a variety of scenarios across the different Windows ML API offerings.

**Image Classification**

A subdomain of computer vision in which an algorithm looks at an image and assigns it a tag from a collection of predefined tags or categories that it has been trained on.

| Windows App Type <br/>Distribution | UWP<br/>In-Box |  UWP<br/>NuGet | Desktop<br/>In-Box | Desktop<br/>NuGet |
|------------|------------------------------------|--------------------------------------|------------------------------------|--------------------------------------|
| [AlexNet](https://github.com/onnx/models/tree/master/vision/classification/alexnet)                                | | ||[✔️C# .NET5 - Samples Gallery](https://github.com/microsoft/Windows-Machine-Learning/tree/master/Samples/WinMLSamplesGallery)<br/>|
| [CaffeNet](https://github.com/onnx/models/tree/master/vision/classification/caffenet)                                  | | ||[✔️C# .NET5 - Samples Gallery](https://github.com/microsoft/Windows-Machine-Learning/tree/master/Samples/WinMLSamplesGallery)<br/>|
| [DenseNet](https://github.com/onnx/models/tree/master/vision/classification/densenet-121)                              | | ||[✔️C# .NET5 - Samples Gallery](https://github.com/microsoft/Windows-Machine-Learning/tree/master/Samples/WinMLSamplesGallery)<br/>|
| [EfficientNet](https://github.com/onnx/models/tree/master/vision/classification/efficientnet-lite4)             | | ||[✔️C# .NET5 - Samples Gallery](https://github.com/microsoft/Windows-Machine-Learning/tree/master/Samples/WinMLSamplesGallery)<br/>|
| [Emoji8](https://blogs.windows.com/windowsdeveloper/2018/11/16/introducing-emoji8/)     | [✔️C#](https://github.com/microsoft/Windows-Machine-Learning/tree/master/Samples/Emoji8/UWP/cs) |                                                 |
| [GoogleNet](https://github.com/onnx/models/tree/master/vision/classification/inception_and_googlenet/googlenet)        | | ||[✔️C# .NET5 - Samples Gallery](https://github.com/microsoft/Windows-Machine-Learning/tree/master/Samples/WinMLSamplesGallery)<br/>|
| [InceptionV1](https://github.com/onnx/models/tree/master/vision/classification/inception_and_googlenet/inception_v1)      | | ||[✔️C# .NET5 - Samples Gallery](https://github.com/microsoft/Windows-Machine-Learning/tree/master/Samples/WinMLSamplesGallery)<br/>|
| [InceptionV2](https://github.com/onnx/models/tree/master/vision/classification/inception_and_googlenet/inception_v2)      | | ||[✔️C# .NET5 - Samples Gallery](https://github.com/microsoft/Windows-Machine-Learning/tree/master/Samples/WinMLSamplesGallery)<br/>|
| [MNIST](https://github.com/onnx/models/tree/master/vision/classification/mnist)      | [✔️C++/CX](https://github.com/Microsoft/Windows-Machine-Learning/tree/master/Samples/MNIST/UWP)<br/>[✔️C#](https://github.com/Microsoft/Windows-Machine-Learning/tree/master/Samples/MNIST/Tutorial/cs)<br/>    |                                                 |
| [MobileNetV2](https://github.com/onnx/models/tree/master/vision/classification/mobilenet)                              | | ||[✔️C# .NET5 - Samples Gallery](https://github.com/microsoft/Windows-Machine-Learning/tree/master/Samples/WinMLSamplesGallery)<br/>|
| [RCNN](https://github.com/onnx/models/tree/master/vision/classification/rcnn_ilsvrc13)                        | | ||[✔️C# .NET5 - Samples Gallery](https://github.com/microsoft/Windows-Machine-Learning/tree/master/Samples/WinMLSamplesGallery)<br/>|
| [ResNet50](https://github.com/onnx/models/tree/master/vision/classification/resnet)                          | | ||[✔️C# .NET5 - Samples Gallery](https://github.com/microsoft/Windows-Machine-Learning/tree/master/Samples/WinMLSamplesGallery)<br/>|
| [ShuffleNetV1](https://github.com/onnx/models/tree/master/vision/classification/shufflenet)                              | | ||[✔️C# .NET5 - Samples Gallery](https://github.com/microsoft/Windows-Machine-Learning/tree/master/Samples/WinMLSamplesGallery)<br/>|
| [ShuffleNetV2](https://github.com/onnx/models/tree/master/vision/classification/shufflenet)                          | | ||[✔️C# .NET5 - Samples Gallery](https://github.com/microsoft/Windows-Machine-Learning/tree/master/Samples/WinMLSamplesGallery)<br/>|
| [SqueezeNet](https://github.com/onnx/models/tree/master/vision/classification/squeezenet) | [✔️C#](https://github.com/Microsoft/Windows-Machine-Learning/tree/master/Samples/SqueezeNetObjectDetection/UWP/cs)<br/>[✔️JavaScript](https://github.com/Microsoft/Windows-Machine-Learning/tree/master/Samples/SqueezeNetObjectDetection/UWP/cs)<br/>        |                     |[✔️C++/WinRT](https://github.com/Microsoft/Windows-Machine-Learning/tree/master/Samples/SqueezeNetObjectDetection/Desktop/cpp)<br/> [✔️C# .NET5](https://github.com/Microsoft/Windows-Machine-Learning/tree/master/Samples/SqueezeNetObjectDetection/NET5)<br/>[✔️C# .NET Core 2](https://github.com/microsoft/Windows-Machine-Learning/tree/master/Samples/SqueezeNetObjectDetection/NETCore/cs)<br/>|[✔️C++/WinRT](https://github.com/Microsoft/Windows-Machine-Learning/tree/master/Samples/SqueezeNetObjectDetection/Desktop/cpp)<br/>[✔️C# .NET5 - Samples Gallery](https://github.com/microsoft/Windows-Machine-Learning/tree/master/Samples/WinMLSamplesGallery)<br/>[✔️Rust](https://github.com/microsoft/Windows-Machine-Learning/tree/master/Samples/RustSqueezenet)<br/>|
| [VGG19](https://github.com/onnx/models/tree/master/vision/classification/vgg)                                          | | ||[✔️C# .NET5 - Samples Gallery](https://github.com/microsoft/Windows-Machine-Learning/tree/master/Samples/WinMLSamplesGallery)<br/>|
| [VGG19bn](https://github.com/onnx/models/tree/master/vision/classification/vgg)                                       | | ||[✔️C# .NET5 - Samples Gallery](https://github.com/microsoft/Windows-Machine-Learning/tree/master/Samples/WinMLSamplesGallery)<br/>|
| [ZFNet512](https://github.com/onnx/models/tree/master/vision/classification/zfnet-512)                                 | | ||[✔️C# .NET5 - Samples Gallery](https://github.com/microsoft/Windows-Machine-Learning/tree/master/Samples/WinMLSamplesGallery)<br/>|

**Style Transfer**

A computer vision technique that allows us to recompose the content of an image in the style of another.

| Windows App Type <br/>Distribution | UWP<br/>In-Box |  UWP<br/>NuGet | Desktop<br/>In-Box | Desktop<br/>NuGet |
|------------|------------------------------------|--------------------------------------|------------------------------------|--------------------------------------|
| FNSCandy   | [✔️C# - FNS Style Transfer](https://github.com/Microsoft/Windows-Machine-Learning/tree/master/Samples/FNSCandyStyleTransfer)<br/>[✔️C# - Real-Time Style Transfer](https://github.com/Microsoft/Windows-Machine-Learning/tree/master/Samples/StyleTransfer)<br/>           |                                                 |

<!--
**Object Detection**


|            | Store App<br/>Inbox API |  Store App<br/>NuGet API | Desktop App<br/>Inbox API |  Desktop App<br/>NuGet API |
|------------|------------------------------------|--------------------------------------|------------------------------------|--------------------------------------|
| [YoloV4](https://github.com/onnx/models/raw/master/vision/object_detection_segmentation/yolov4/model/yolov4.onnx)                         | | ||[✔️C# .NET5 - Samples Gallery](https://github.com/microsoft/Windows-Machine-Learning/tree/master/Samples/WinMLSamplesGallery)<br/>|

-->

## Advanced Scenario Samples

These advanced samples show how to use various binding and evaluation features in Windows ML:

- **[Custom Tensorization](https://github.com/Microsoft/Windows-Machine-Learning/tree/master/Samples/CustomTensorization)**: a Windows Console Application (C++/WinRT) that shows how to do custom tensorization.
- **[Custom Operator (CPU)](https://github.com/Microsoft/Windows-Machine-Learning/tree/master/Samples/CustomOperator)**: a desktop app that defines multiple custom cpu operators. One of these is a debug operator which we invite you to integrate into your own workflow.
- **[Adapter Selection](https://github.com/Microsoft/Windows-Machine-Learning/tree/master/Samples/AdapterSelection)**: a desktop app that demonstrates how to choose a specific device adapter for running your model
- **[Plane Identifier](https://github.com/Microsoft/Windows-AppConsult-Samples-UWP/tree/master/PlaneIdentifier)**: a UWP app and a WPF app packaged with the Desktop Bridge, sharing the same model trained using [Azure Custom Vision service](https://customvision.ai/). For step-by-step instructions for this sample, please see the blog post [Upgrade your WinML application to the latest bits](https://blogs.msdn.microsoft.com/appconsult/2018/11/06/upgrade-your-winml-application-to-the-latest-bits/).
- **[Custom Vision and Windows ML](https://github.com/Microsoft/Windows-AppConsult-Samples-UWP/tree/master/PlaneIdentifier)**: The tutorial shows how to train a neural network model to classify images of food using Azure Custom Vision service, export the model to ONNX format, and deploy the model in a Windows Machine Learning application running locally on Windows device. 
- **[ML.NET and Windows ML](https://github.com/Microsoft/Windows-AppConsult-Samples-UWP/tree/master/PlaneIdentifier)**: This tutorial shows you how to train a neural network model to classify images of food using ML.NET Model Builder, export the model to ONNX format, and deploy the model in a Windows Machine Learning application running locally on a Windows device. 
- **[PyTorch Data Analysis](https://github.com/Microsoft/Windows-AppConsult-Samples-UWP/tree/master/PlaneIdentifier)**: The tutorial shows how to solve a classification task with a neural network using the PyTorch library, export the model to ONNX format and deploy the model with the Windows Machine Learning application that can run on any Windows device.
- **[PyTorch Image Classification](https://github.com/Microsoft/Windows-AppConsult-Samples-UWP/tree/master/PlaneIdentifier)**: The tutorial shows how to train an image classification neural network model using PyTorch, export the model to the ONNX format, and deploy it in a Windows Machine Learning application running locally on your Windows device.
- **[YoloV4 Object Detection](https://github.com/Microsoft/Windows-AppConsult-Samples-UWP/tree/master/PlaneIdentifier)**: This tutorial shows how to build a UWP C# app that uses the YOLOv4 model to detect objects in video streams.

## Developer Tools

- **Model Conversion**

  Windows ML provides inferencing capabilities powered by the ONNX Runtime engine. As such, all models run in Windows ML must be converted to the [ONNX Model format](https://github.com/onnx/onnx). Models built and trained in source frameworks like TensorFlow or PyTorch must be converted to ONNX. Check out the documentation for how to convert to an ONNX model:
    - https://onnxruntime.ai/docs/tutorials/mobile/model-conversion.html
    - https://docs.microsoft.com/en-us/windows/ai/windows-ml/tutorials/pytorch-convert-model
    - [WinMLTools](https://pypi.org/project/winmltools/): a Python tool for converting models from different machine learning toolkits into ONNX for use with Windows ML.

- **Model Optimization**

  Models may need further optimizations applied post conversion to support advanced features like batching and quantization. Check out the following tools for optimizing your model:

  - **[WinML Dashboard (Preview)](https://github.com/Microsoft/Windows-Machine-Learning/tree/master/Tools/WinMLDashboard)**: a GUI-based tool for viewing, editing, converting, and validating machine learning models for Windows ML inference engine. This tool can be used to enable free dimensions on models that were built with fixed dimensions. [Download Preview Version](https://github.com/microsoft/Windows-Machine-Learning/releases)


  - **[Graph Optimizations](https://onnxruntime.ai/docs/performance/graph-optimizations.html#:~:text=ONNX%20Runtime%20provides%20various%20graph%20optimizations%20to%20improve,to%20more%20complex%20node%20fusions%20and%20layout%20optimizations):** Graph optimizations are essentially graph-level transformations, ranging from small graph simplifications and node eliminations to more complex node fusions and layout optimizations.

  - **[Graph Quantization](https://onnxruntime.ai/docs/performance/quantization.html#:~:text=Quantization%20in%20ONNX%20Runtime%20refers%20to%208%20bit,the%20floating%20point%20numbers%20to%20a%20quantization%20space.)**: Quantization in ONNX Runtime refers to 8 bit linear quantization of an ONNX model.

- **Model Validation**
  - **[WinMLRunner](https://github.com/Microsoft/Windows-Machine-Learning/tree/master/Tools/WinMLRunner)**: a command-line tool that can run .onnx or .pb models where the input and output variables are tensors or images. It is a very handy tool to quickly validate an ONNX model. It will attempt to load, bind, and evaluate a model and print out helpful messages. It also captures performance measurements.

    [Download x64 Exe](https://github.com/microsoft/Windows-Machine-Learning/releases)

- **Model Integration**
  - **[WinML Code Generator (mlgen)](https://marketplace.visualstudio.com/items?itemName=WinML.mlgen)**: a Visual Studio extension to help you get started using WinML APIs on UWP apps by generating a template code when you add a trained ONNX file into the UWP project. From the template code you can load a model, create a session, bind inputs, and evaluate with wrapper codes. See [docs](https://docs.microsoft.com/en-us/windows/ai/mlgen) for more info.

    Download for [VS 2017](https://marketplace.visualstudio.com/items?itemName=WinML.mlgen), [VS 2019](https://marketplace.visualstudio.com/items?itemName=WinML.MLGenV2)

  - **[WinML Samples Gallery](https://github.com/microsoft/Windows-Machine-Learning/tree/master/Samples/WinMLSamplesGallery):** explore a variety of ML integration scenarios and models.

  - Check out the [Model Samples](#model-samples) and [Advanced Scenario Samples](#advanced-scenarios) to learn how to use Windows ML in your application.
  



## Feedback
- For issues, file a bug on [GitHub Issues](https://github.com/Microsoft/Windows-Machine-Learning/issues).
- Ask questions on [Stack Overflow](https://stackoverflow.com/questions/tagged/windows-machine-learning).
- Vote for popular feature requests on [Windows Developer Feedback](https://wpdev.uservoice.com/forums/110705-universal-windows-platform?category_id=341035) or include your own request.

## External Links
 - [ONNX Runtime: cross-platform, high performance ML inferencing and training accelerator](https://github.com/microsoft/onnxruntime/).
 - [ONNX: Open Neural Network Exchange Project](https://onnx.ai/).

## Contributing

We're always looking for your help to fix bugs and improve the samples. Create a pull request, and we'll be happy to take a look.

This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/).
For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or
contact [opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments.
