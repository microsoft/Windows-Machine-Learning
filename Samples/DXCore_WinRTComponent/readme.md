# DXCore WinRT Helper sample

In Windows 10 20H1, you must use the native C++ DXCore APIs to access compute accelerators in your WinML application. This sample demonstrates an easier method to consume DXCore from within a UWP app written in C#, JavaScript, or any other supported language. It does so by wrapping the DXCore native code in a Windows Runtime Component; the WinRT Component implements helper APIs which initialize a WinML LearningModelDevice using DXCore.

*About prerelease APIs:** Support for compute accelerators and the DXCore API are in developer preview during 20H1. Functionality, performance and reliability are all incomplete and the API surface may change.

## About the AI on PC Developer Kit

The AI on PC Developer Kit is a complete kit for building deep learning inference applications on a PC. It provides a cost-competitive way to build and run AI applications using Intel® Movidius™ Vision Processing Units (VPUs), Intel® Processors and Intel® Graphics Technology (GPUs) on a laptop PC with WinML and OpenVINO™ toolkit  and delivers low-power image processing, computer vision, and deep learning inferencing for exceptional performance per watt, per dollar.

Any usage that need continuous periods of deep learning operation with the lowest possible impact to battery life is a good usage target for the Development kit.  Some examples include are semantic segmentation on video conferencing, object detection, visual log-in, visual transformations, Emoji and Avatar creation, among others. Certain usages that need compute that is not available on CPU and GPU because they are pre occupied with other tasks can be off loaded to the VPU. 

The developer kit comes with Windows® 10 and several pre-installed tools including Microsoft WinML, Intel® Distribution of OpenVINO™ toolkit, the Intel® Distribution for Python* and Microsoft Visual Studio* 2019 for an IDE. Additionally, the kit provides several Getting Started Guides, code samples, tutorials and sample applications for WinML and OpenVINO™ to help accelerate your development.

## Prerequisites

- [Visual Studio 2019 Version 16.0.0 or Newer](https://visualstudio.microsoft.com/)
- [Windows 10 - Build 18925 or higher](https://www.microsoft.com/en-us/software-download/windowsinsiderpreviewiso)
- [Windows SDK - Build 18925 or higher](https://www.microsoft.com/en-us/software-download/windowsinsiderpreviewSDK)
- Visual Studio Extension for C++/WinRT

## Build the sample

1. If you download the samples ZIP, be sure to unzip the entire archive, not just the folder with the sample you want to build.
2. Start Microsoft Visual Studio 2019 and select **File > Open > Project/Solution**.
3. Starting in the folder where you unzipped the samples, go to the **Samples** subfolder, then the subfolder for this specific sample (**DXCore_WinRTComponent**). Double-click the Visual Studio solution file (.sln).
4. Confirm that the project is pointed to the correct SDK that you installed (e.g. 18870). You can do this by right-clicking the project in the **Solution Explorer**, selecting **Properties**, and modifying the **Windows SDK Version**.
5. Confirm that you are set for the right configuration and platform (for example: Debug, x64).
6. Build the solution (**Ctrl+Shift+B**).

## Run the sample

The sample includes two projects:

- **DXCore_WinRTComponent:** C++ implementation of the WinRT Component helper which calls into DXCore and projects helper methods to initialize WinML. This project produces a DLL and does not perform any action on its own.
- **TesterApp:** A  minimal C# UWP app which calls into the WinRT Component to access various hardware accelerators via DXCore. Click the button to exercise the WinRT Component's functionality.

To consume the WinRT Component in your own application:

1. Add the WinRT Component project to your app's solution.
2. [Add a reference](https://docs.microsoft.com/en-us/visualstudio/ide/managing-references-in-a-project) from your app's project to the WinRT Component.
3. Your app now has access to the projected APIs provided by the WinRT Component, as defined in DXCoreHelper.idl.

## License

MIT. See [LICENSE file](https://github.com/Microsoft/Windows-Machine-Learning/blob/master/LICENSE).