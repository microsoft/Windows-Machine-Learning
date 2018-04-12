# Windows ML Explorer sample

Windows ML Explorer is sample app that you can use to bootstrap ML models to be evaluated with Windows ML. Currently, the app includes a circuit board defect detection model that can detect defects on images and a real-time camera feed of a printed circuit board.

To read more about this sample application, see [How three lines of code and Windows Machine Learning empower .NET developers to run AI locally on Windows 10 devices](https://blogs.technet.microsoft.com/machinelearning/2018/03/13/how-three-lines-of-code-and-windows-machine-learning-empower-net-developers-to-run-ai-locally-on-windows-10-devices/).

To learn more about Windows ML development, see [Windows ML documentation](https://docs.microsoft.com/windows/uwp/machine-learning/).

## Prerequisites

- [Windows 10 - Build 17110 or higher](https://www.microsoft.com/en-us/software-download/windowsinsiderpreviewiso)
- [Windows SDK - Build 17110 or higher](https://www.microsoft.com/en-us/software-download/windowsinsiderpreviewSDK)

## Build the sample

1. If you download the samples ZIP, be sure to unzip the entire archive, not just the folder with
   the sample you want to build.

2. Start Microsoft Visual Studio 2017 and select **File** \> **Open** \> **Project/Solution**.

3. Starting in the folder where you unzipped the samples, go to the **Samples** subfolder, then the
   **UWP** subfolder for the platform, then **WinMLExplorer** subfolder for this sample application.
	Double-click the Visual Studio Solution (.sln) file.

4. Press Ctrl+Shift+B, or select **Build** \> **Build Solution**.

## Run the sample

The next steps depend on whether you just want to deploy the sample or you want to both deploy and run it.

### Deploy the sample

- Select Build > Deploy Solution.

### Deploy and run the sample

- To debug the sample and then run it, press F5 or select Debug >  Start Debugging. To run the sample without debugging, press Ctrl+F5 or selectDebug > Start Without Debugging.

## License

MIT. See [LICENSE file](https://github.com/Microsoft/Windows-Machine-Learning/blob/master/LICENSE).