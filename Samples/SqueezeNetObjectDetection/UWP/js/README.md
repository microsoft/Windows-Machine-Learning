
# SqueezeNet Object Detection sample

This UWP application uses SqueezeNet, a pre-trained machine learning model, to detect the predominant object in an image selected by the user from a file.

This sample demonstrates the use of the [Windows.AI.MachineLearning](https://docs.microsoft.com/uwp/api/windows.ai.machinelearning) API to load a model, bind an input image and an output tensor, and evaluate a binding.

If you would like to use a different model, then you can use [Netron](https://github.com/lutzroeder/Netron) to determine the input and output requirements of your ONNX model.

## Prerequisites


> NOTE:  there is a bug in the JavaScript system that prevents JS from calling into Windows.AI.MachineLearning.   Once this fix has made it into a Windows Insider release we will update this page.  For now , you can use this code as reference, but it will not run yet.


- [Visual Studio 2017 Version 15.7.4 or Newer](https://developer.microsoft.com/en-us/windows/downloads)
- [Windows 10 - Build XXXX or higher](https://www.microsoft.com/en-us/software-download/windowsinsiderpreviewiso)
- [Windows SDK - Build 17738 or higher](https://www.microsoft.com/en-us/software-download/windowsinsiderpreviewSDK)

## Build the sample

1. If you download the samples ZIP, be sure to unzip the entire archive, not just the folder with
   the sample you want to build.

2. Start Microsoft Visual Studio 2017 and select **File** \> **Open** \> **Project/Solution**.

3. Starting in the folder where you unzipped the samples, go to the Samples subfolder, then the
   subfolder for this specific sample. Double-click the Visual Studio solution file (.sln) file.

4. Change the platform target default to x64 or x86 if you want to test on a non-ARM device.

5. Press Ctrl+Shift+B, or select **Build** \> **Build Solution**.

## Run the sample

The next steps depend on whether you just want to deploy the sample or you want to both deploy and run it.

### Deploying the sample

- Select Build > Deploy Solution.

### Deploying and running the sample

- To debug the sample and then run it, press F5 or select Debug >  Start Debugging. To run the sample without debugging, press Ctrl+F5 or selectDebug > Start Without Debugging.

## License

MIT. See [LICENSE file](https://github.com/Microsoft/Windows-Machine-Learning/blob/master/LICENSE).
