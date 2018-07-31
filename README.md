# Windows ML samples

This repo contains sample apps that demonstrate how to use Windows ML to build machine learning applications for Windows 10.

For tutorials, how-tos, and additional information, see the [Windows ML documentation](https://review.docs.microsoft.com/en-us/windows/ai/?branch=master):

## Pre-release APIs

These samples use the [Windows.AI.MachineLearning APIs](https://docs.microsoft.com/uwp/api/windows.ai.machinelearning), a pre-released product which may be substantially modified before it’s commercially released. Microsoft makes no warranties, express or implied, with respect to the information provided here.

## Sample apps

These generic examples show how to use various models and input feeds with Windows ML. We have both C++ native apps and C# UWP samples

- **FNSCandyStyleTransfer\UWP\cs**: a UWP C# app that uses the FNS-Candy style transfer model to make a cool image.
- **SqueezeNetObjectDetection\UWP\cs**: a UWP C# app that uses the SqueezeNet model to detect the predominant object in an image.
- **SqueezeNetObjectDetection\Desktop\cpp**: a classic desktop C++/WinRT app that uses the SqueezeNet model to detect the predominant object in an image.
- **MNIST\UWP\cs**: a UWP C# app that uses the MNIST model to detect numberic characters.
- **MNIST\UWP\cppcx**: a UWP C++/CX app that uses the MNIST model to detect numberic characters.

## Requirements

- [Visual Studio 2017 Version 15.7.4 or Newer](https://developer.microsoft.com/en-us/windows/downloads)
- [Windows 10 - Build 17724 or higher](https://www.microsoft.com/en-us/software-download/windowsinsiderpreviewiso)
- [Windows SDK - Build 17723 or higher](https://www.microsoft.com/en-us/software-download/windowsinsiderpreviewSDK)

## Using the samples
The easiest way to use these samples without using Git is to download the zip file containing the current version (using the following link or by clicking the "Download ZIP" button on the repo page). You can then unzip the entire archive and use the samples in Visual Studio 2017.

[Download the samples ZIP](https://github.com/Microsoft/Windows-Machine-Learning/archive/RS5.zip)

Notes:
Before you unzip the archive, right-click it, select Properties, and then select Unblock.
Be sure to unzip the entire archive, and not just individual samples. The samples all depend on the SharedContent folder in the archive.
In Visual Studio 2017, the platform target defaults to ARM, so be sure to change that to x64 or x86 if you want to test on a non-ARM device.

Reminder: If you unzip individual samples, they will not build due to references to other portions of the ZIP file that were not unzipped. You must unzip the entire archive if you intend to build the samples.

## Contributing

We're always looking for your help to fix bugs and improve the samples. Create a pull request, and we'll be happy to take a look.

This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/).
For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or
contact [opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments.
