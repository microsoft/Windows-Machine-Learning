# DXCore WinRT Helper sample

In Windows 10 20H1, you must use the native C++ DXCore APIs to access compute accelerators in your WinML application. This sample demonstrates an easier method to consume DXCore from within a UWP app written in C#, JavaScript, or any other supported language. It does so by wrapping the DXCore native code in a Windows Runtime Component; the WinRT Component implements helper APIs which initialize a WinML LearningModelDevice using DXCore.

*About prerelease APIs:** Support for compute accelerators and the DXCore API are in developer preview during 20H1. Functionality, performance and reliability are all incomplete and the API surface may change.

## Run the sample

The DXCore WinRT Component project is intended to be consumed by another project containing the actual WinML application; for example the MNIST UWP sample application.

To consume the WinRT Component in your own application:

1. Copy the files for the WinRT Component project into your app's solution.
*Note:* This project needs to be stored in a subfolder of the app's solution, for example if the SLN file is in `MySolutionFolder\MySolution.sln`, you should copy the WinRT Component project files to `MySolutionFolder\DXCore_WinRTComponent\`. This is because the project relies the C++/WinRT Nuget package and expects the package binaries to be located in a specific relative location.
2. [Add a reference](https://docs.microsoft.com/en-us/visualstudio/ide/managing-references-in-a-project) from your app's project to the WinRT Component.
3. Your app now has access to the projected APIs provided by the WinRT Component, as defined in DXCoreHelper.idl.

## License

MIT. See [LICENSE file](https://github.com/Microsoft/Windows-Machine-Learning/blob/master/LICENSE).