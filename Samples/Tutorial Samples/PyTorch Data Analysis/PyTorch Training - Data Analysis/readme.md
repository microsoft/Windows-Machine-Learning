# Readme

This sample represents the finished state of a data analyis sample code, as part of a tutorial presented in the Windows ML developer documentation.


The tutorial shows how to solve a classification task with a neural network using the PyTorch library, export the model to ONNX format and deploy the model with the Windows Machine Learning application that can run on any Windows device. 


Basic knowledge in Python and C# programming languages is required for the tutorial. Previous experience in machine learning is preferable but not required.

## Scenario 

In this tutorial, we'll create a machine learning data analysis application to predict the type of Iris flowers. For this purpose, you will use Fisher’s Iris flower dataset. The model will be trained to recognize certain types of Irises patterns and predict the correct type. 
  

## Prerequisites for PyTorch - model training:

PyTorch is supported on the following Windows distributions: 

* Windows 7 and greater. Windows 10 or greater recommended. 
* Windows Server 2008 r2 and greater 

To use Pytorch on Windows, you must have Python 3.x installed. Python 2.x is not supported. 

## Prerequisites for Windows ML app deployment

To deploy this app or follow the associated tutorial, you'll need the following:

*	Windows 10 version 1809 (build 17763) or higher. You can check your build version number by running `winver` via the Run command `(Windows logo key + R)`.
*	Windows SDK for build 17763 or higher. [You can get the SDK here.](https://developer.microsoft.com/windows/downloads/windows-10-sdk/)
*	Visual Studio 2017 version 15.7 or later. We recommend using Visual Studio 2019, and some screenshots in this tutorial may be different if you use VS2017 instead. [You can get Visual Studio here.](https://developer.microsoft.com/windows/downloads/)
*	You'll also need to [enable Developer Mode on your PC](https://docs.microsoft.com/windows/apps/get-started/enable-your-device-for-development)

> [!NOTE]
> Windows ML APIs are built into the latest versions of Windows 10 (1809 or higher) and Windows Server 2019. If your target platform is older versions of Windows, you can [port your WinML app to the redistributable NuGet package (Windows 8.1 or higher)](https://docs.microsoft.com/windows/ai/windows-ml/port-app-to-nuget). 