# Load Model from Embedded Resource
This application shows how to take an embedded resource that contains an ONNX model and convert it to a stream that can then be passed to the LearningModel constructor.

## Prerequisites

- [Visual Studio 2017 Version 15.7.4 or Newer](https://developer.microsoft.com/en-us/windows/downloads)
- [Windows 10 - Build 17763 or higher](https://www.microsoft.com/en-us/software-download/windowsinsiderpreviewiso)
- [Windows SDK - Build 17763 or higher](https://www.microsoft.com/en-us/software-download/windowsinsiderpreviewSDK)
- Visual Studio Extension for C++/WinRT

  Do the following to add the C++/WinRT extension in Visual Studio.
  1. Go to **Tools > Extensions and Updates**. 
  2. Select **Online** in the left pane and search for "WinRT" using the search box.
  3. Select the **C++/WinRT** extension, click **Download**, and close Visual Studio. The extension should install automatically.
  4. When the extension has finished installing, re-open Visual Studio.

## Build the sample

1. If you download the samples ZIP, be sure to unzip the entire archive, not just the folder with the sample you want to build.
2. Start Microsoft Visual Studio 2017 and select **File > Open > Project/Solution**.
3. Starting in the folder where you unzipped the samples, go to the **Samples** subfolder, then the subfolder for this specific sample (**StreamFromResource**). Double-click the Visual Studio solution file (.sln).
4. Confirm that the project is pointed to the correct SDK that you installed (e.g. 17763). You can do this by right-clicking the project in the **Solution Explorer**, selecting **Properties**, and modifying the **Windows SDK Version**.
5. Confirm that you are set for the right configuration and platform (for example: Debug, x64).
6. Build the solution (**Ctrl+Shift+B**).

## Run the sample
It will just return `0` if everything works. Otherwise there are error codes that can be searched for in `main.cpp`. 


## License

MIT. See [LICENSE file](https://github.com/Microsoft/Windows-Machine-Learning/blob/master/LICENSE).