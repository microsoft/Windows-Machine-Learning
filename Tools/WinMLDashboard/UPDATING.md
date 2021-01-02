# Updating WinMLDashboard

WinMLDashboard should be updated in cadence with updates to WinML and OnnxRuntime.

## Steps to update WinMLDashboard:
1) a. Update the following dependencies in requirements.txt to their latest stable versions:
    - onnxmltools
    - onnxconverter-common
    - tf2onnx
    - keras2onnx
    - skl2onnx

   b. You can optionally update these ML framework dependencies to their latest version, but this can be unstable and isn't    essential to justify delaying updates to WinMLDashboard.
    - coremltools_windows
    - tensorflow
    - Keras
    - Keras-Applications
    - Keras-Preprocessing
    - scikit-learn
    - xgboost
    - lightgbm
    - libsvm

2) Update MicrosoftMLRunner binary to the latest release. Using MicrosoftMLRunner rather than WinMLRunner allows us to utilize opset support from the latest WinML/Onnxruntime nuget release. The latest MicrosoftMLRunner release is available [here](https://github.com/microsoft/Windows-Machine-Learning/releases).
3) Update that the latest WinML, DirectML, and Onnxruntime binaries to their latest versions. Microsoft.AI.MachineLearning.dll and onnxruntime.dll are available in this [package](https://www.nuget.org/packages/Microsoft.AI.MachineLearning/), and DirectML.dll is available in this [package](https://www.nuget.org/packages/Microsoft.AI.DirectML/). These binaries are also present in the MicrosoftMLRunner package and can be used from there.
4) Update the opset options in the UI to the latest supported by WinML. This is done by adding to the ONNXVersionOptions object in [View.tsx](src/view/convert/View.tsx). WinML's latest supported opset information is available [here](https://github.com/microsoft/onnxruntime/blob/master/docs/Versioning.md).
5) Verify that the CI automated pipeline has succeeded when opening a pull request. This pipeline builds WinMLDashboard and runs basic UI tests.
5) Manually ensure that conversion from each framework succeeds.Test collateral for each framework can be found in test/convert_collateral. Make sure to recreate the local python environment after updating dependency versions. For the tensorflow output names specify: MobilenetV1/Predictions/Reshape_1:0
6) Issue a new official release of WinMLDashboard on the release page of this repository.