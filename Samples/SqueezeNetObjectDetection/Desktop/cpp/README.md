
# SqueezeNet Object Detection sample

This is a desktop application that uses SqueezeNet, a pre-trained machine learning model, to detect the predominant object in an image selected by the user from a file.

(fill in more)

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
3. Starting in the folder where you unzipped the samples, go to the Samples subfolder, then the subfolder for this specific sample.    Double-click the Visual Studio project file (.csproj) file.
Press Ctrl+Shift+B, or select Build > Build Solution

## Run the sample

1. Open a Command Window
2. Change current folder to the folder containing the built exe
3. Run the executable as shown below:

SqueezeNetObjectionDetection.exe model.onnx kitten_224.png

## License

MIT. See [LICENSE file](https://github.com/Microsoft/Windows-Machine-Learning/blob/master/LICENSE).
