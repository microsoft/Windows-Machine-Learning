# Windows ML samples

This repo contains sample apps that demonstrate how to use Windows ML to build machine learning applications for Windows 10.

For tutorials, how-tos, and additional information, see the Windows ML documentation:
https://docs.microsoft.com/windows/uwp/machine-learning

## Pre-release APIs

These samples use the [Windows.AI.MachineLearning APIs](https://docs.microsoft.com/uwp/api/windows.ai.machinelearning), a pre-released product which may be substantially modified before it’s commercially released. Microsoft makes no warranties, express or implied, with respect to the information provided here.

## Sample apps

These generic examples show how to use various models and input feeds with Windows ML. We have both C++ native apps and C# UWP samples

- **Desktop\SqueezeNet**: a native C++ desktop app that usesthe SqueezeNet model to detect the predominant object in an image.
- **Store\SqueezeNet**: a UWP C# store app that uses the SqueezeNet model to detect the predominant object in an image.
- **Store\SnapCandy**: a UWP C# store app that uses the FnsCandy model to make cool images

## Requirements

- [Windows 10 - Build 17713 or higher](https://www.microsoft.com/en-us/software-download/windowsinsiderpreviewiso)
- [Windows SDK - Build 17713 or higher](https://www.microsoft.com/en-us/software-download/windowsinsiderpreviewSDK)

## Contributing

We're always looking for your help to fix bugs and improve the samples. Create a pull request, and we'll be happy to take a look.

This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/).
For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or
contact [opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments.
