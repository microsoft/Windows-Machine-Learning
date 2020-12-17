# Updating WinMLDashboard

WinMLDashboard should be updated in cadence with updates to WinML and OnnxRuntime.

## Steps to update WinMLDashboard:
1) Update the following dependencies in requirements.txt to their latest stable versions:
    - coremltools_windows
    - onnxmltools
    - onnxconverter-common
    - tensorflow
    - tf2onnx
    - Keras
    - Keras-Applications
    - Keras-Preprocessing
    - keras2onnx
    - scikit-learn
    - xgboost
    - lightgbm
    - libsvm

2) Update MicrosoftMLRunner binary to the latest release. Using MicrosoftMLRunner rather than WinMLRunner allows us to utilize opset support from the latest WinML/Onnxruntime nuget release. The latest MicrosoftMLRunner release is available [here](https://github.com/microsoft/Windows-Machine-Learning/releases).
3) Update that the latest WinML, DirectML, and Onnxruntime binaries to their latest versions. Microsoft.AI.MachineLearning.dll and onnxruntime.dll are available in this [package](https://www.nuget.org/packages/Microsoft.AI.MachineLearning/), and DirectML.dll is available in this [package](https://www.nuget.org/packages/Microsoft.AI.DirectML/).
4) Update the opset options in the UI to the latest supported by WinML. This is done by adding to the ONNXVersionOptions object in [View.tsx](src/view/convert/View.tsx). WinML's latest supported opset information is available [here](https://github.com/microsoft/onnxruntime/blob/master/docs/Versioning.md).
5) Verify that the CI automated pipeline has succeeded when opening a pull request. This pipeline builds WinMLDashboard and runs basic UI tests.
5) Manually ensure that conversion from each framework succeeds.Test collateral for each framework can be found [here](test/convert_collateral). For the tensorflow output names specify: MobilenetV1/Predictions/Reshape_1:0