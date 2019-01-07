# Custom Operator Sample

This is a desktop application that uses models containing custom operator definitions and implementations.
The necessary code is included to define and register these custom operators. API reference:  https://docs.microsoft.com/en-us/windows/ai/custom-operators

## Included custom operators
There are three custom operators included in this sample: Relu, NoisyRelu and Debug.
The Relu custom operator is replacing an existing operator while NoisyRelu and Debug are new operators.

Relu transforms the input data using the function max(0, input). Noisy Relu is a variant of Relu which introduces Guassian noise.
For more information on these functions: 
https://en.wikipedia.org/wiki/Rectifier_(neural_networks)#Noisy_ReLUs

The Debug custom operator is designed to help with debugging intermediate outputs. The operator and how to include it in your own workflow is described in detail here: 
[debug_readme.md](../../debug_readme.md)

The Relu and NoisyRelu operator client code curates its own input data, but the Debug operator client code accepts an input image and runs a modified SqueezeNet model.

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
3. Starting in the folder where you unzipped the samples, go to the **Samples** subfolder, then the subfolder for this specific sample (**custom-operator-cpu-sample\desktop\cpp**). Double-click the Visual Studio solution file (.sln).
4. Confirm that the project is pointed to the correct SDK that you installed (e.g. 17763). You can do this by right-clicking the project in the **Solution Explorer**, selecting **Properties**, and modifying the **Windows SDK Version**.
5. Confirm that you are set for the right configuration and platform (for example: Debug, x64).
6. Build the solution (**Ctrl+Shift+B**).

## Run the sample

1. Open a Command Prompt (in the Windows 10 search bar, type **cmd** and press **Enter**).
2. Change the current folder to the folder containing the built EXE (`cd <path-to-exe>`).
3. Run the executable as shown below. Make sure to replace the install location with what matches yours:
  ```
  custom-operator-cpu-sample.exe debug <path to image file>
  custom-operator-cpu-sample.exe relu
  custom-operator-cpu-sample.exe noisyrelu
  ```

## Expected outputs
  Debug (with kitten_224.png as input): 
  ```
  Creating the custom operator provider.
  Calling LoadFromFilePath('squeezenet_debug_one_output.onnx').
  Creating ModelSession.
  Loading the image...
  Binding...
  Running the model...intermediate debug operators will be output to specified file paths
  model run took 203 ticks
  tabby, tabby cat with confidence of 0.931461
  Egyptian cat with confidence of 0.065307
  tiger cat with confidence of 0.002927
  ```
  Relu:
  ```
  Creating the custom operator provider.
  Calling LoadFromFilePath('relu.onnx').
  Creating ModelSession.
  Create the ModelBinding binding collection.
  Create the input tensor.
  Binding input tensor to the ModelBinding binding collection.
  Create the output tensor.
  Binding output tensor to the ModelBinding binding collection.
  Calling EvaluateSync()
  Getting output binding (Y), featureKind=0, dataKind=1, dims=1
  Got output binding data, size=(5).
  0.000000
  0.000000
  0.000000
  25.000000
  50.000000
  Done
  ```
  NoisyRelu:
  ```
  Creating the custom operator provider.
  Calling LoadFromFilePath('noisy_relu.onnx').
  Creating ModelSession.
  Create the ModelBinding binding collection.
  Create the input tensor.
  Binding input tensor to the ModelBinding binding collection.
  Create the output tensor.
  Binding output tensor to the ModelBinding binding collection.
  Calling EvaluateSync()
  Getting output binding (Y), featureKind=0, dataKind=1, dims=1
  Got output binding data, size=(5).
  0.000000
  0.000000
  0.000000
  23.406185
  50.008259
  Done
  ```

## License

MIT. See [LICENSE file](https://github.com/Microsoft/Windows-Machine-Learning/blob/master/LICENSE).
