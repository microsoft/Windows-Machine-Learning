# Windows Machine Learning

Windows Machine Learning is a high-performance machine learning inference API that is powered by [ONNX Runtime](https://onnxruntime.ai/) and [DirectML](https://docs.microsoft.com/en-us/windows/ai/directml/dml).


[![Alt text](https://i.ytimg.com/vi/8MCDSlm326U/hq720.jpg?sqp=-oaymwEcCOgCEMoBSFXyq4qpAw4IARUAAIhCGAFwAcABBg==&rs=AOn4CLCFThAhMuWU3UrHtPjc4Ttz9SSkpQ)](https://www.youtube.com/watch?v=8MCDSlm326U)

The Windows ML API is a [Windows Runtime Component](https://docs.microsoft.com/en-us/windows/uwp/winrt-components/) and is suitable for high-performance, low-latency applications such as frameworks, games, and other real-time applications as well as applications built with high-level languages.


This repo contains Windows Machine Learning samples and tools that demonstrate how to build machine learning powered scenarios into Windows applications.

- [Getting Started with Windows ML](#getting-started)
- [Model Demo Samples](#model-demos)
- [Advanced Scenario Samples](#advanced-scenarios)
- [Developer Tools](#tools)
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

Windows ML offers machine learning inferencing via the inbox Windows SDK as well as a redistributable NuGet package. The table below highlights the availability, distribution, language support, servicing, and forward compatibility aspects of the In-Box and NuGet package for Windows ML.

| |In-Box|	NuGet|
|-|-----|------|
|Availability|	Windows 10 version 1809 or higher	| Windows 8.1 or higher
|Distribution|	Built into the Windows SDK |	Package and distribute as part of your application
|Servicing|	Microsoft-driven (customers benefit automatically)	| Developer-driven
|Forward| compatibility	Automatically rolls forward with new features	| Developer needs to update package manually

Learn mode [here](https://docs.microsoft.com/en-us/windows/ai/windows-ml/get-started).

- [Visual Studio 2017 Version 15.7.4 or Newer](https://developer.microsoft.com/en-us/windows/downloads)
- [Windows 10 - Build 17763 (RS5) or Newer](https://www.microsoft.com/en-us/software-download/windows10)
- [Windows SDK - Build 17763 (RS5) or Newer](https://www.microsoft.com/en-us/software-download/windowsinsiderpreviewSDK)


## Model Samples
In this section you will find various model samples for a variety of scenarios across the different Windows ML API offerings. 

**Image Classification**

A subdomain of computer vision in which an algorithm looks at an image and assigns it a tag from a collection of predefined tags or categories that it has been trained on.

|            | Store App<br/>Windows.AI.MachineLearning<br/>(Inbox API) |  Store App<br/>Microsoft.AI.MachineLearning<br/>(Redist API) | Desktop App<br/>Windows.AI.MachineLearning<br/>(Inbox API) |  Desktop App<br/>Microsoft.AI.MachineLearning<br/>(Redist API) |
|------------|------------------------------------|--------------------------------------|------------------------------------|--------------------------------------|
| [AlexNet](https://github.com/onnx/models/raw/master/vision/classification/alexnet/model/bvlcalexnet-9.onnx)                                | | ||[✔️C# .NET5 - Samples Gallery](https://github.com/Microsoft/Windows-Machine-Learning/tree/master/Samples/SqueezeNetObjectDetection/NET5)<br/>|
| [CaffeNet](https://github.com/onnx/models/raw/master/vision/classification/caffenet/model/caffenet-9.onnx)                                  | | ||[✔️C# .NET5 - Samples Gallery](https://github.com/Microsoft/Windows-Machine-Learning/tree/master/Samples/SqueezeNetObjectDetection/NET5)<br/>|
| [DenseNet](https://github.com/onnx/models/raw/master/vision/classification/densenet-121/model/densenet-9.onnx)                              | | ||[✔️C# .NET5 - Samples Gallery](https://github.com/Microsoft/Windows-Machine-Learning/tree/master/Samples/SqueezeNetObjectDetection/NET5)<br/>|
| [EfficientNet](https://github.com/onnx/models/raw/master/vision/classification/efficientnet-lite4/model/efficientnet-lite4-11.onnx)             | | ||[✔️C# .NET5 - Samples Gallery](https://github.com/Microsoft/Windows-Machine-Learning/tree/master/Samples/SqueezeNetObjectDetection/NET5)<br/>|
| Emoji8     | [✔️C#](https://github.com/Microsoft/Windows-Machine-Learning/tree/master/Samples/MNIST/UWP) |                                                 |
| [GoogleNet](https://github.com/onnx/models/raw/master/vision/classification/inception_and_googlenet/googlenet/model/googlenet-9.onnx)        | | ||[✔️C# .NET5 - Samples Gallery](https://github.com/Microsoft/Windows-Machine-Learning/tree/master/Samples/SqueezeNetObjectDetection/NET5)<br/>|
| [InceptionV1](https://github.com/onnx/models/raw/master/vision/classification/inception_and_googlenet/inception_v1/model/inception-v1-9.)      | | ||[✔️C# .NET5 - Samples Gallery](https://github.com/Microsoft/Windows-Machine-Learning/tree/master/Samples/SqueezeNetObjectDetection/NET5)<br/>|
| [InceptionV2](https://github.com/onnx/models/raw/master/vision/classification/inception_and_googlenet/inception_v2/model/inception-v2-9.)      | | ||[✔️C# .NET5 - Samples Gallery](https://github.com/Microsoft/Windows-Machine-Learning/tree/master/Samples/SqueezeNetObjectDetection/NET5)<br/>|
| [MNIST](https://github.com/onnx/models/tree/master/vision/classification/mnist)      | [✔️C++/CX](https://github.com/Microsoft/Windows-Machine-Learning/tree/master/Samples/MNIST/UWP)<br/>[✔️C#](https://github.com/Microsoft/Windows-Machine-Learning/tree/master/Samples/MNIST/Tutorial/cs)<br/>    |                                                 |
| [MobileNetV2](https://github.com/onnx/models/raw/master/vision/classification/mobilenet/model/mobilenetv2-7.onnx)                              | | ||[✔️C# .NET5 - Samples Gallery](https://github.com/Microsoft/Windows-Machine-Learning/tree/master/Samples/SqueezeNetObjectDetection/NET5)<br/>|
| [RCNN](https://github.com/onnx/models/raw/master/vision/classification/rcnn_ilsvrc13/model/rcnn-ilsvrc13-9.onnx)                        | | ||[✔️C# .NET5 - Samples Gallery](https://github.com/Microsoft/Windows-Machine-Learning/tree/master/Samples/SqueezeNetObjectDetection/NET5)<br/>|
| [ResNet50](https://github.com/onnx/models/raw/master/vision/classification/resnet/model/resnet50-caffe2-v1-9.onnx)                          | | ||[✔️C# .NET5 - Samples Gallery](https://github.com/Microsoft/Windows-Machine-Learning/tree/master/Samples/SqueezeNetObjectDetection/NET5)<br/>|
| [ShuffleNetV1](https://github.com/onnx/models/raw/master/vision/classification/shufflenet/model/shufflenet-9.onnx)                              | | ||[✔️C# .NET5 - Samples Gallery](https://github.com/Microsoft/Windows-Machine-Learning/tree/master/Samples/SqueezeNetObjectDetection/NET5)<br/>|
| [ShuffleNetV2](https://github.com/onnx/models/raw/master/vision/classification/shufflenet/model/shufflenet-v2-10.onnx)                          | | ||[✔️C# .NET5 - Samples Gallery](https://github.com/Microsoft/Windows-Machine-Learning/tree/master/Samples/SqueezeNetObjectDetection/NET5)<br/>|
| [SqueezeNet](https://github.com/onnx/models/raw/master/vision/classification/squeezenet/model/squeezenet1.1-7.onnx) | [✔️C#](https://github.com/Microsoft/Windows-Machine-Learning/tree/master/Samples/SqueezeNetObjectDetection/UWP/cs)<br/>[✔️JavaScript](https://github.com/Microsoft/Windows-Machine-Learning/tree/master/Samples/SqueezeNetObjectDetection/UWP/cs)<br/>        |                     |[✔️C++/WinRT](https://github.com/Microsoft/Windows-Machine-Learning/tree/master/Samples/SqueezeNetObjectDetection/Desktop/cpp)<br/> [✔️C# .NET5](https://github.com/Microsoft/Windows-Machine-Learning/tree/master/Samples/SqueezeNetObjectDetection/NET5)<br/>[✔️C# .NET Core 2](https://github.com/microsoft/Windows-Machine-Learning/tree/master/Samples/SqueezeNetObjectDetection/NETCore/cs)<br/>|[✔️C++/WinRT](https://github.com/Microsoft/Windows-Machine-Learning/tree/master/Samples/SqueezeNetObjectDetection/Desktop/cpp)<br/>[✔️C# .NET5 - Samples Gallery](https://github.com/Microsoft/Windows-Machine-Learning/tree/master/Samples/SqueezeNetObjectDetection/NET5)<br/>|
| [VGG19](https://github.com/onnx/models/raw/master/vision/classification/vgg/model/vgg19-7.onnx)                                          | | ||[✔️C# .NET5 - Samples Gallery](https://github.com/Microsoft/Windows-Machine-Learning/tree/master/Samples/SqueezeNetObjectDetection/NET5)<br/>|
| [VGG19bn](https://github.com/onnx/models/raw/master/vision/classification/vgg/model/vgg19-bn-7.onnx)                                       | | ||[✔️C# .NET5 - Samples Gallery](https://github.com/Microsoft/Windows-Machine-Learning/tree/master/Samples/SqueezeNetObjectDetection/NET5)<br/>|
| [ZFNet512](https://github.com/onnx/models/raw/master/vision/classification/zfnet-512/model/zfnet512-9.onnx)                                 | | ||[✔️C# .NET5 - Samples Gallery](https://github.com/Microsoft/Windows-Machine-Learning/tree/master/Samples/SqueezeNetObjectDetection/NET5)<br/>|

**Style Transfer**

A computer vision technique that allows us to recompose the content of an image in the style of another.

|            | Store App<br/>Windows.AI.MachineLearning<br/>(Inbox API) |  Store App<br/>Microsoft.AI.MachineLearning<br/>(Redist API) | Desktop App<br/>Windows.AI.MachineLearning<br/>(Inbox API) |  Desktop App<br/>Microsoft.AI.MachineLearning<br/>(Redist API) |
|------------|------------------------------------|--------------------------------------|------------------------------------|--------------------------------------|
| FNSCandy   | [✔️C# (FNS Style Transfer)](https://github.com/Microsoft/Windows-Machine-Learning/tree/master/Samples/FNSCandyStyleTransfer)<br/>[✔️C# (Real-Time Style Transfer)](https://github.com/Microsoft/Windows-Machine-Learning/tree/master/Samples/StyleTransfer)<br/>           |                                                 |

<!--
**Object Detection**


|            | Store App<br/>Windows.AI.MachineLearning<br/>(Inbox API) |  Store App<br/>Microsoft.AI.MachineLearning<br/>(Redist API) | Desktop App<br/>Windows.AI.MachineLearning<br/>(Inbox API) |  Desktop App<br/>Microsoft.AI.MachineLearning<br/>(Redist API) |
|------------|------------------------------------|--------------------------------------|------------------------------------|--------------------------------------|
| [YoloV4](https://github.com/onnx/models/raw/master/vision/object_detection_segmentation/yolov4/model/yolov4.onnx)                         | | ||[✔️C# .NET5 - Samples Gallery](https://github.com/Microsoft/Windows-Machine-Learning/tree/master/Samples/SqueezeNetObjectDetection/NET5)<br/>|

-->

## Advanced Scenario Samples

These advanced samples show how to use various binding and evaluation features feeds in Windows ML:

- **[Custom Tensorization](https://github.com/Microsoft/Windows-Machine-Learning/tree/master/Samples/CustomTensorization)**: a Windows Console Application (C++/WinRT) that shows how to do custom tensorization.
- **[Custom Operator (CPU)](https://github.com/Microsoft/Windows-Machine-Learning/tree/master/Samples/CustomOperator)**: a desktop app that defines multiple custom cpu operators. One of these is a debug operator which we invite you to integrate into your own workflow.
- **[Adapter Selection](https://github.com/Microsoft/Windows-Machine-Learning/tree/master/Samples/AdapterSelection)**: a desktop app that demonstrates how to choose a specific device adapter for running your model
- **[Plane Identifier](https://github.com/Microsoft/Windows-AppConsult-Samples-UWP/tree/master/PlaneIdentifier)**: a UWP app and a WPF app packaged with the Desktop Bridge, sharing the same model trained using [Azure Custom Vision service](https://customvision.ai/). For step-by-step instructions for this sample, please see the blog post [Upgrade your WinML application to the latest bits](https://blogs.msdn.microsoft.com/appconsult/2018/11/06/upgrade-your-winml-application-to-the-latest-bits/).

## Developer Tools

- **Model Conversion**

  Windows ML provides inferencing capabilities powered by the ONNX Runtime engine. As such, all models run in Windows ML must be converted to the [ONNX Model format](https://github.com/onnx/onnx). Models built and trained in source frameworks like TensorFlow or PyTorch must be converted to ONNX. Check out the documentation for how to convert to an ONNX model:
    - https://onnxruntime.ai/docs/tutorials/mobile/model-conversion.html
    - https://docs.microsoft.com/en-us/windows/ai/windows-ml/tutorials/pytorch-convert-model
    - [WinMLTools](https://pypi.org/project/winmltools/): a Python tool for converting models from different machine learning toolkits into ONNX for use with Windows ML.
- **Model Optimization**

  Models may need further optimizations applied post conversion to support advanced features like batching and quantization. Check out the following tools for optimizig your model:

  - **[WinML Dashboard (Preview)](https://github.com/Microsoft/Windows-Machine-Learning/tree/master/Tools/WinMLDashboard)**: a GUI-based tool for viewing, editing, converting, and validating machine learning models for Windows ML inference engine. This tool can be used to enable free dimensions on models that were built with fixed dimensions. [Download Preview Version](https://github.com/microsoft/Windows-Machine-Learning/releases)


  - **[Graph Optimizations](https://onnxruntime.ai/docs/performance/graph-optimizations.html#:~:text=ONNX%20Runtime%20provides%20various%20graph%20optimizations%20to%20improve,to%20more%20complex%20node%20fusions%20and%20layout%20optimizations):** Graph optimizations are essentially graph-level transformations, ranging from small graph simplifications and node eliminations to more complex node fusions and layout optimizations.

  - **[Graph Quantization](https://onnxruntime.ai/docs/performance/quantization.html#:~:text=Quantization%20in%20ONNX%20Runtime%20refers%20to%208%20bit,the%20floating%20point%20numbers%20to%20a%20quantization%20space.)**: Quantization in ONNX Runtime refers to 8 bit linear quantization of an ONNX model.

- **Model Validation**
  - **[WinMLRunner](https://github.com/Microsoft/Windows-Machine-Learning/tree/master/Tools/WinMLRunner)**: a command-line tool that can run .onnx or .pb models where the input and output variables are tensors or images. It is a very handy tool to quickly validate an ONNX model. It will attempt to load, bind, and evaluate a model and print out helpful messages. It also captures performance measurements.

    [Download x64 Exe](https://github.com/microsoft/Windows-Machine-Learning/releases)

- **Model Integration**
  - **[WinML Code Generator (mlgen)](https://marketplace.visualstudio.com/items?itemName=WinML.mlgen)**: a Visual Studio extension to help you get started using WinML APIs on UWP apps by generating a template code when you add a trained ONNX file into the UWP project. From the template code you can load a model, create a session, bind inputs, and evaluate with wrapper codes. See [docs](https://docs.microsoft.com/en-us/windows/ai/mlgen) for more info.

    Download for [VS 2017](https://marketplace.visualstudio.com/items?itemName=WinML.mlgen), [VS 2019](https://marketplace.visualstudio.com/items?itemName=WinML.MLGenV2)

  - Check out the [Model Demo Samples](#model-demos) and [Advanced Scenario Samples](#advanced-scenarios).



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
