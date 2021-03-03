# SqueezeNet Object Detection sample

This is a .NET Core 2 application that uses SqueezeNet, a pre-trained machine learning model, to detect the predominant object in an image selected by the user from a file.

Note: SqueezeNet was trained to work with image sizes of 224x224, so you must provide an image of size 224X224.
Also, the asynchronous handlers defined in the code are required due to a limitation of .NET Core 2. With the release of .NET Core 3 you will be able to use the async/await pattern.

To get access to Windows.AI.MachineLearning and various other Windows classes an assembly reference needs to be added for Windows.winmd
For this project the assembly reference is parametrized by the environment variable WINDOWS_WINMD, so you need to set this environment variable before building.
The file path for the Windows.winmd file may be: ```C:\Program Files (x86)\Windows Kits\10\UnionMetadata\[version]\Windows.winmd```


## Prerequisites

- [Visual Studio 2017 Version 15.7.4 or Newer](https://developer.microsoft.com/en-us/windows/downloads)
- [Windows 10 - Build 17763 or higher](https://www.microsoft.com/en-us/software-download/windowsinsiderpreviewiso)
- [Windows SDK - Build 17763 or higher](https://www.microsoft.com/en-us/software-download/windowsinsiderpreviewSDK)

## Build the sample

1. If you download the samples ZIP, be sure to unzip the entire archive, not just the folder with the sample you want to build.
2. Start Microsoft Visual Studio 2017 and select **File > Open > Project/Solution**.
3. Starting in the folder where you unzipped the samples, go to the **Samples** subfolder, then the subfolder for this sample (**SqueezeNetObjectDetection). Double-click the Visual Studio solution file (.sln).
4. Confirm that you are set for the right configuration and platform (for example: Debug, x64).
5. Build the solution by right clicking the project in **Solution Explorer** and selecting Build (**Ctrl+Shift+B**).

## Running the sample

- To debug the sample and then run it, press F5 or select Debug >  Start Debugging. To run the sample without debugging, press Ctrl+F5 or selectDebug > Start Without Debugging.

- You should get output similar to the following:
  ```
  Loading modelfile 'C:\Repos\Windows-Machine-Learning\SharedContent\models\SqueezeNet.onnx' on the 'default' device
  model file loaded in 421 ticks
  Loading the image...
  Binding...
  Running the model...
  model run took 31 ticks
  "tabby, tabby cat" with confidence of 0.931461
  "Egyptian cat" with confidence of 0.065307
  "tiger cat" with confidence of 0.002927
  ```

## License

MIT. See [LICENSE file](https://github.com/Microsoft/Windows-Machine-Learning/blob/master/LICENSE).