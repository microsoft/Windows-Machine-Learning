
# SqueezeNet Object Detection sample

This is a desktop application that uses SqueezeNet, a pre-trained machine learning model, to detect the predominant object in an image selected by the user from a file.

Note: SqueezeNet was trained to work with image sizes of 224x224

## Prerequisites

- [Windows 10 - Build 17724 or higher](https://www.microsoft.com/en-us/software-download/windowsinsiderpreviewiso)
- [Windows SDK - Build 17724 or higher](https://www.microsoft.com/en-us/software-download/windowsinsiderpreviewSDK)

You'll also need to add the C++/WinRT extension in Visual Studio.
1. Go to Tools > Extensions and Updates. 
2. Select "Online" in left pane and Search for "WinRT" using search box
3. Click download, and restart Visual Studio

## Build the sample

1. If you download the samples ZIP, be sure to unzip the entire archive, not just the folder with the sample you want to build.
2. Start Microsoft Visual Studio 2017 and select File > Open > Project/Solution.
3. Starting in the folder where you unzipped the samples, go to the Samples subfolder, then the subfolder for this specific sample.    Double-click the Visual Studio solution file (.sln) file.
4. Confirm that the project is pointed to the correct SDK that you installed (e.g. 17724)
5. Confirm that you are set for the right configuration and platform (e.g. Debug, x64)
6. Build the solution
7. Update the command arguments to match the sample below
8. Run the sample ! (F5)

## Run the sample

1. Open a Command Window
2. Change current folder to the folder containing the built exe
3. Run the executable as shown below, make sure to replace the install location with what matches yours

`.\SqueezeNetObjectDetection.exe F:\src\github\microsoft\Windows-Machine-Learning\SharedContent\models\SqueezeNet.onnx  F:\src\github\microsoft\Windows-Machine-Learning\SharedContent\media\kitten_224.png`

## License

MIT. See [LICENSE file](https://github.com/Microsoft/Windows-Machine-Learning/blob/master/LICENSE).
