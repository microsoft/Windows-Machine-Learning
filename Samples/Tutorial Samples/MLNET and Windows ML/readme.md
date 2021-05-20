# Readme

This sample represents the finished state of a tutorial presented in the Windows ML developer documentation.

This tutorial shows you how to train a neural network model to classify images of food using ML.NET Model Builder, export the model to ONNX format, and deploy the model in a Windows Machine Learning application running locally on a Windows device. No previous expertise in machine learning is required, and we'll guide you step by step through the process. 

## Scenario

In this tutorial, we'll create a machine learning food classification application that runs on Windows devices. The model will be trained to recognize certain types of patterns to classify an image of food, and when given an image will return a classification tag and the associated percentage confidence value of that classification.

### Prerequisites for model training

To build and train your model, you'll use the ML.NET Model Buider in Visual Studio.

* You'll need Visual Studio 2019 16.6.1 or later to use a ML.NET Model Builder. [You can get Visual Studio here.](https://developer.microsoft.com/windows/downloads/)
* You'll need an Azure account to train a model with ML.NET Model Builder within the Azure ML Workspace. If you're new to Azure, you may sign up for an [Azure free account](https://azure.microsoft.com/free/services/machine-learning/).

> [!NOTE]
> Interested in learning more about Azure sign-up options and Azure free accounts? Check out [Create an Azure account](https://docs.microsoft.com/learn/modules/create-an-azure-account/) on Microsoft Learn.

ML.NET Model Builder is an intuitive graphical Visual Studio extension, used to build, train, and deploy custom machine learning models. It uses automated machine learning (AutoML) to explore different machine learning algorithms and settings to help you find the one that best suits your scenario.

ML.NET Model Builder ships with Visual Studio version 16.6.1 or later, when you install one of the .NET workloads. Make sure to have the ML.NET Model Builder component checked in the installer when you download or modify Visual Studio. To check if your VS has the ML.NET Model Builder components, go to Extensions and select Manage Extensions. Type Model Builder in the search bar to review the extension results. 

ML.NET Model Builder is currently a Preview feature. So, in order to use the tool, in Visual Studio you must go to Tools > Options > Environment > Preview Features and enable ML.NET Model Builder:

> [!NOTE]
> Interested in learning more about ML.NET Model Builder and different scenarios it supports? Please review the [Model Builder documentation.](https://docs.microsoft.com/dotnet/machine-learning/automate-training-with-model-builder)

### Prerequisites for Windows ML app deployment

To create and deploy a Widows ML app, you'll need the following: 

*	Windows 10 version 1809 (build 17763) or higher. You can check your build version number by running `winver` via the Run command `(Windows logo key + R)`.
*	Windows SDK for build 17763 or higher. [You can get the SDK here.](https://developer.microsoft.com/windows/downloads/windows-10-sdk/)
*	Visual Studio 2019 version 16.6.1 or later. [You can get Visual Studio here.](https://developer.microsoft.com/windows/downloads/)
*	Windows ML Code Generator (mlgen) Visual Studio extension. Download for [VS 2019](https://marketplace.visualstudio.com/items?itemName=WinML.mlgenv2).
*	If you decide to create a UWP app, you'll need to enable the Universal Windows Platform development workload in Visual Studio.
*	You'll also need to [enable Developer Mode on your PC](https://docs.microsoft.com/windows/apps/get-started/enable-your-device-for-development)

> [!NOTE]
> Windows ML APIs are built into the latest versions of Windows 10 (1809 or higher) and Windows Server 2019. If your target platform is older versions of Windows, you can [port your WinML app to the redistributable NuGet package (Windows 8.1 or higher)](https://docs.microsoft.com/windows/ai/windows-ml/port-app-to-nuget). 