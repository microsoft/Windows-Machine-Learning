# Updating WinMLRunner

WinMLRunner should be updated in cadence with updates to Windows Machine Learning and ONNX Runtime.

## Steps to update WinMLRunner:
1) Update the Microsoft.AI.MachineLearning package in packages.config to its latest release.
2) Ensure all relevant vcxproj files reference the updated package path. These files are:
  a) WinMLRunner.vcxproj
  b) WinMLRunnerScenarios.vcxproj
  c) WinMLRunnerStaticLib.vcxproj
3) Ensure WinMLRunner builds and perform some basic selfhosting, like running any of the models in the SharedContent folder.
4) Issue a new official release of WinMLRunner on the repository release page [here](https://github.com/microsoft/Windows-Machine-Learning/releases).