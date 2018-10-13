| Sample/Tool | Status |
|---------|--------------|
| All Samples | [![Build status](https://mscodehub.visualstudio.com/DefaultCollection/WindowsAI/_apis/build/status/winml/WinML%20Dashboard%20CI%20Build)](https://mscodehub.visualstudio.com/DefaultCollection/WindowsAI/_build/latest?definitionId=602) |
| WinmlRunner | [![Build status](https://mscodehub.visualstudio.com/DefaultCollection/WindowsAI/_apis/build/status/winml/WinML%20Dashboard%20CI%20Build)](https://mscodehub.visualstudio.com/DefaultCollection/WindowsAI/_build/latest?definitionId=602) |

# Windows ML

Welcome to the Windows ML repo! Windows ML allows you to use trained machine learning models in your Windows apps (C#, C++, Javascript). The Windows ML inference engine evaluates trained models locally on Windows devices. Hardware optimizations for CPU and GPU additionally enable high performance for quick evaluation results.

In this repo, you will find sample apps that demonstrate how to use Windows ML to build machine learning applications, and tools that help verify models and troubleshoot issues during development on Windows 10. 

For additional information on Windows ML, including step-by-step tutorials and how-to guides, please visit the [Windows ML documentation](https://docs.microsoft.com/en-us/windows/ai/).

## Pre-release APIs

These samples use the [Windows.AI.MachineLearning APIs](https://docs.microsoft.com/uwp/api/windows.ai.machinelearning), a pre-released product which may be substantially modified before it’s commercially released. Microsoft makes no warranties, express or implied, with respect to the information provided here.

## Requirements

- [Visual Studio 2017 Version 15.7.4 or Newer](https://developer.microsoft.com/en-us/windows/downloads)
- [Windows 10 - Build 17738 or higher](https://www.microsoft.com/en-us/software-download/windowsinsiderpreviewiso)
- [Windows SDK - Build 17738 or higher](https://www.microsoft.com/en-us/software-download/windowsinsiderpreviewSDK)

## Sample apps

These generic examples show how to use various models and input feeds with Windows ML. We have both C++ native desktop apps and C# and Javascript UWP samples

- **[FNSCandyStyleTransfer\UWP\cs](https://github.com/Microsoft/Windows-Machine-Learning/tree/master/Samples/FNSCandyStyleTransfer)**: a UWP C# app that uses the FNS-Candy style transfer model to make a cool image.
- **[SqueezeNetObjectDetection\UWP\cs](https://github.com/Microsoft/Windows-Machine-Learning/tree/master/Samples/SqueezeNetObjectDetection/UWP/cs)**: a UWP C# app that uses the SqueezeNet model to detect the predominant object in an image.
- **[SqueezeNetObjectDetection\UWP\js](https://github.com/Microsoft/Windows-Machine-Learning/tree/master/Samples/SqueezeNetObjectDetection/UWP/js)**: a UWP Javascript app that uses SqueezeNet model to detect the predominent object in an image. 
- **[SqueezeNetObjectDetection\Desktop\cpp](https://github.com/Microsoft/Windows-Machine-Learning/tree/master/Samples/SqueezeNetObjectDetection/Desktop/cpp)**: a classic desktop C++/WinRT app that uses the SqueezeNet model to detect the predominant object in an image.
- **[MNIST\UWP\cs](https://github.com/Microsoft/Windows-Machine-Learning/tree/master/Samples/MNIST/Tutorial/cs)**: a UWP C# app that uses the MNIST model to detect numberic characters.
- **[MNIST\UWP\cppcx](https://github.com/Microsoft/Windows-Machine-Learning/tree/master/Samples/MNIST/UWP)**: a UWP C++/CX app that uses the MNIST model to detect numberic characters.

## Developer Tools
- **[WinMLRunner](https://github.com/Microsoft/Windows-Machine-Learning/tree/master/Tools/WinMLRunner)**: a command-line tool that can run .onnx or .pb models where the input and output variables are tensors or images. It is a very handy tool to quickly validate an ONNX model. It will attempt to load, bind, and evaluate a model and print out helpful messages. It also captures performance measurements. 

## Using the samples
The easiest way to use these samples without using Git is to download the zip file containing the current version (using the following link or by clicking the "Download ZIP" button on the repo page). You can then unzip the entire archive and use the samples in Visual Studio 2017.

[Download the samples ZIP](https://github.com/Microsoft/Windows-Machine-Learning/archive/master.zip)

Notes:
Before you unzip the archive, right-click it, select Properties, and then select Unblock.
Be sure to unzip the entire archive, and not just individual samples. The samples all depend on the SharedContent folder in the archive.
In Visual Studio 2017, the platform target defaults to ARM, so be sure to change that to x64 or x86 if you want to test on a non-ARM device.

Reminder: If you unzip individual samples, they will not build due to references to other portions of the ZIP file that were not unzipped. You must unzip the entire archive if you intend to build the samples.

## Feedback
- For issues file a bug on [GitHub Issues](https://github.com/Microsoft/Windows-Machine-Learning/issues).
- Ask questions on [Stack Overflow](https://stackoverflow.com/questions/tagged/windows-machine-learning).
- Vote for popular feature requests on [Windows Developer Feedback](https://wpdev.uservoice.com/forums/110705-universal-windows-platform?category_id=341035) or include your own request.

## Release Notes
**Build 17723**
- Requires ONNX v1.2 or higher.
- Supports F16 datatypes with GPU-based model inferences for better performance and reduced model footprint. You can use [WinMLTools](https://pypi.org/project/winmltools/) to convert your models from FP32 to FP16.
- Allows desktop apps to consume Windows.AI.MachineLearning APIs with WinRT/C++.

## Related Projects
 - [ONNX: Open Neural Network Exchange Project](https://onnx.ai/).
 - [WinMLTools](https://pypi.org/project/winmltools/).

## Contributing

We're always looking for your help to fix bugs and improve the samples. Create a pull request, and we'll be happy to take a look.

This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/).
For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or
contact [opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments.
