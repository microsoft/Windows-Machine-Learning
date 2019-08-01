# Adapter Selection sample

This is a desktop application that demonstrates using DXCore as the replacement for DXGI. This sample is set up to use new compute-only adapters, such as the Intel MyriadX VPU (Vision Processing Unit), and run a SqueezeNet image detection model using the selected device.

**About prerelease APIs:** Support for compute-only adapters and the DXCore API are in developer preview for 20H1. Functionality, performance and reliability are all incomplete and the API surface may change.

Note: SqueezeNet was trained to work with image sizes of 224x224, so you must provide an image of size 224X224.

## About the AI on PC Developer Kit 
The AI on PC Developer Kit is a complete kit for building deep learning inference applications on a PC. It provides a cost-competitive way to build and run AI applications using Intel® Movidius™ Vision Processing Units (VPUs), Intel® Processors and Intel® Graphics Technology (GPUs) on a laptop PC with WinML and OpenVINO™ toolkit  and delivers low-power image processing, computer vision, and deep learning inferencing for exceptional performance per watt, per dollar. 

Any usage that need continuous periods of deep learning operation with the lowest possible impact to battery life is a good usage target for the Development kit.  Some examples include are semantic segmentation on video conferencing, object detection, visual log-in, visual transformations, Emoji and Avatar creation, among others. Certain usages that need compute that is not available on CPU and GPU because they are pre occupied with other tasks can be off loaded to the VPU.  

The developer kit comes with Windows® 10 and several pre-installed tools including Microsoft WinML, Intel® Distribution of OpenVINO™ toolkit, the Intel® Distribution for Python* and Microsoft Visual Studio* 2019 for an IDE. Additionally, the kit provides several Getting Started Guides, code samples, tutorials and sample applications for WinML and OpenVINO™ to help accelerate your development. 

## Prerequisites

- [Visual Studio 2019 Version 16.0.0 or Newer](https://visualstudio.microsoft.com/)
- [Windows 10 - Build 18908 or higher](https://www.microsoft.com/en-us/software-download/windowsinsiderpreviewiso)
- [Windows SDK - Build 18908 or higher](https://www.microsoft.com/en-us/software-download/windowsinsiderpreviewSDK)

## Run the sample
Usage:
```
SqueezeNetObjectDetection_MCDM_AdapterSelection.exe [options]
options:
  -Model <full path to model>: Model Path (Only FP16 models)
  -Image <full path to image>: Image Path
  -SelectAdapter : Toggle select adapter functionality to select the device to run sample on.
```

1. Open a Command Prompt (in the Windows 10 search bar, type **cmd** and press **Enter**).
2. Change the current folder to the folder containing the built EXE (`cd <path-to-exe>`).
3. Run the executable as shown below. Make sure to replace the install location with what matches yours:
  ```
  SqueezeNetObjectDetection_MCDM_AdapterSelection.exe
  ```
You should get output similar to the following:
  ```
  Index: 0, Description: Intel(R) Iris(R) Plus Graphics 650
  Index: 2, Description: Intel(R) VPU Aceelerator 2485
Movidius Compiler (moviCompile) v00.95.0 Build 3157 Alpha #1. Restricted distribution.
Successfully created LearningModelDevice with selected Adapter
Loading modelfile '.\SqueezeNet_fp16.onnx' on the selected device
model file loaded in 16 ticks
Movidius Compiler (moviCompile) v00.95.0 Build 3157 Alpha #1. Restricted distribution.
Movidius Compiler (moviCompile) v00.95.0 Build 3157 Alpha #1. Restricted distribution.
Movidius Compiler (moviCompile) v00.95.0 Build 3157 Alpha #1. Restricted distribution.
Movidius Compiler (moviCompile) v00.95.0 Build 3157 Alpha #1. Restricted distribution.
Movidius Compiler (moviCompile) v00.95.0 Build 3157 Alpha #1. Restricted distribution.
Movidius Compiler (moviCompile) v00.95.0 Build 3157 Alpha #1. Restricted distribution.
Movidius Compiler (moviCompile) v00.95.0 Build 3157 Alpha #1. Restricted distribution.
Movidius Compiler (moviCompile) v00.95.0 Build 3157 Alpha #1. Restricted distribution.
Movidius Compiler (moviCompile) v00.95.0 Build 3157 Alpha #1. Restricted distribution.
Movidius Compiler (moviCompile) v00.95.0 Build 3157 Alpha #1. Restricted distribution.
Loading the image '.\kitten_224.png' ...
Binding...
Running the model...
model run took 562 ticks
tench, Tinca tinca with confidence of 0.738499
barracouta, snoek with confidence of 0.254980
gar, garfish, garpike, billfish, Lepisosteus osseus with confidence of 0.002547
  ```



## License

MIT. See [LICENSE file](https://github.com/Microsoft/Windows-Machine-Learning/blob/master/LICENSE).