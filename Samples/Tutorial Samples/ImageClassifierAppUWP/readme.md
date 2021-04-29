# Readme

This sample represents the finished state of a tutorial presented in the Windows ML developer documentation.

The tutorial shows how to train a neural network model to classify images of food using Azure Custom Vision service, export the model to ONNX format, and deploy the model in a Windows Machine Learning application running locally on Windows device. No previous expertise in machine learning is required! We will guide you step by step through the process. 

## Scenario

In this tutorial, we'll create a machine learning food classification application that runs on Windows devices. The model will be trained to recognize certain types of patterns to classify an image of food, and when given an image will return a classification tag and the associated percentage confidence value of that classification.

### Prerequisites for model training

To build and train a model, you'll need a subscription to Azure Custom Vision services.

If you're new to Azure, you may sign up for an [Azure free account](https://azure.microsoft.com/free/services/machine-learning/). This will give you an opportunity to build, train, and deploy machine learning models with Azure AI. 

> [!NOTE]
> Interested in learning more about Azure sign-up options and Azure free accounts? Check out [Create an Azure account](https://docs.microsoft.com/learn/modules/create-an-azure-account/) on Microsoft Learn.

### Prerequisites for Windows ML app deployment

To deploy this app or follow the associated tutorial, you'll need the following:

*	Windows 10 version 1809 (build 17763) or higher. You can check your build version number by running `winver` via the Run command `(Windows logo key + R)`.
*	Windows SDK for build 17763 or higher. [You can get the SDK here.](https://developer.microsoft.com/windows/downloads/windows-10-sdk/)
*	Visual Studio 2017 version 15.7 or later. We recommend using Visual Studio 2019, and some screenshots in this tutorial may be different if you use VS2017 instead. [You can get Visual Studio here.](https://developer.microsoft.com/windows/downloads/)
*	Windows ML Code Generator (mlgen) Visual Studio extension. Download for [VS 2019](https://marketplace.visualstudio.com/items?itemName=WinML.mlgenv2) or for [VS 2017](https://marketplace.visualstudio.com/items?itemName=WinML.mlgen).
*	If you decide to create a UWP app, you'll need to enable the Universal Windows Platfurm development workload in Visual Studio.
*	You'll also need to [enable Developer Mode on your PC](https://docs.microsoft.com/windows/apps/get-started/enable-your-device-for-development)

> [!NOTE]
> Windows ML APIs are built into the latest versions of Windows 10 (1809 or higher) and Windows Server 2019. If your target platform is older versions of Windows, you can [port your WinML app to the redistributable NuGet package (Windows 8.1 or higher)](../port-app-to-nuget.md). 