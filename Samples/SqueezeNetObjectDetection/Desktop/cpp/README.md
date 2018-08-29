# SqueezeNet Object Detection sample

This is a desktop application that uses SqueezeNet, a pre-trained machine learning model, to detect the predominant object in an image selected by the user from a file.

Note: SqueezeNet was trained to work with image sizes of 224x224, so you must provide an image of size 224X224.

## Prerequisites

- [Visual Studio 2017 Version 15.7.4 or Newer](https://developer.microsoft.com/en-us/windows/downloads)
- [Windows 10 - Build 17738 or higher](https://www.microsoft.com/en-us/software-download/windowsinsiderpreviewiso)
- [Windows SDK - Build 17738 or higher](https://www.microsoft.com/en-us/software-download/windowsinsiderpreviewSDK)
- Visual Studio Extension for C++/WinRT

  Do the following to add the C++/WinRT extension in Visual Studio.
  1. Go to **Tools > Extensions and Updates**. 
  2. Select **Online** in the left pane and search for "WinRT" using the search box.
  3. Select the **C++/WinRT** extension, click **Download**, and close Visual Studio. The extension should install automatically.
  4. When the extension has finished installing, re-open Visual Studio.

## Build the sample

1. If you download the samples ZIP, be sure to unzip the entire archive, not just the folder with the sample you want to build.
2. Start Microsoft Visual Studio 2017 and select **File > Open > Project/Solution**.
3. Starting in the folder where you unzipped the samples, go to the **Samples** subfolder, then the subfolder for this specific sample (**SqueezeNetObjectDetection\Desktop\cpp**). Double-click the Visual Studio solution file (.sln).
4. Confirm that the project is pointed to the correct SDK that you installed (e.g. 17738). You can do this by right-clicking the project in the **Solution Explorer**, selecting **Properties**, and modifying the **Windows SDK Version**.
5. Confirm that you are set for the right configuration and platform (for example: Debug, x64).
6. Build the solution (**Ctrl+Shift+B**).

## Run the sample

1. Make sure **Labels.txt** is copied into the folder with the built executable.
2. Open a Command Prompt (in the Windows 10 search bar, type **cmd** and press **Enter**).
3. Change the current folder to the folder containing the built EXE (`cd <path-to-exe>`).
4. Run the executable as shown below. Make sure to replace the install location with what matches yours:
  ```
  SqueezeNetObjectDetection.exe C:\Repos\Windows-Machine-Learning\SharedContent\models\SqueezeNet.onnx C:\Repos\Windows-Machine-Learning\SharedContent\media\kitten_224.png
  ```
5. You should get output similar to the following:
  ```
  Loading modelfile 'C:\Repos\Windows-Machine-Learning\SharedContent\models\SqueezeNet.onnx' on the 'default' device
  model file loaded in 421 ticks
  Loading the image...
  Binding...
  Running the model...
  model run took 31 ticks
  tabby, tabby cat with confidence of 0.931461
  Egyptian cat with confidence of 0.065307
  Persian cat with confidence of 0.000193
  ```

## License

MIT. See [LICENSE file](https://github.com/Microsoft/Windows-Machine-Learning/blob/master/LICENSE).
