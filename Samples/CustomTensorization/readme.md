# Custom Tensorization Sample

## Overview

This sample tells how to tensorize the input image by using WinML public APIs on both CPU and GPU.

## Background

Typically, when doing input image binding, we would

1. Load the image from disk.
2. Convert the input to [SoftwareBitmap](https://docs.microsoft.com/en-us/uwp/api/windows.graphics.imaging.softwarebitmap)(CPU resource) or [IDXGISurface](https://docs.microsoft.com/en-us/windows/desktop/api/dxgi/nn-dxgi-idxgisurface)(GPU resource).
3. Convert to [VideoFrame](https://docs.microsoft.com/en-us/uwp/api/Windows.Media.VideoFrame).
4. Convert [ImageFeatureValue](https://docs.microsoft.com/en-us/uwp/api/windows.ai.machinelearning.imagefeaturevalue).
5. Use the VideoFrame or ImageFeatureValue as bind object.

In this way, we are using Internal APIs to do CPU/GPU tensorization during these process.

However, in some cases,
1. People do not have to use VideoFrame. They could put their input image into CPU/GPU resource directly.
2. People want to preprocess their input data, like normalizing the pixel values  from range 0-255 to range 0-1 during tensorization

So this sample gives a way of how to manually tensorize input image data with WinML public APIs.


## Assumptions
   1. This samples supposes that we are using a model(fns-candy.onnx) that takes input in the format of BGR.
   2. This samples supposes the pixel range is 0-255.
   3. d3dx12.h need to be downloaded from [Microsoft/DirectX-Graphics-Samples](https://github.com/Microsoft/DirectX-Graphics-Samples/blob/master/Libraries/D3DX12/d3dx12.h) and is included in helper.cpp. Windows SDK contains Direct3D SDK but not the d3dx12.h headers.

## Steps to run the sample

1. Load the `CustomTensorization.sln` into Visual Studio
2. Build and run the solution
3. Check the output images, named as `output_cpu.png` and `output_gpu.png`, in the same folder.

> For better understanding of how to tensorize manually on CPU or GPU, Going through the comments and code in `helper.*` is expected to be necessary.

## Code Understanding

Basically, this sample encapsulates two chunks of code, tensorization on CPU and GPU separately. In order to make it runnable, we levaraged the tutorial about how to write a WinML desktop appliction. 

1. main.cpp
    > follows the tutorial to create a windows machine learning desktop application on [public doc](https://docs.microsoft.com/en-us/windows/ai/get-started-desktop).
    
    ```C++
    void BindModel(
        VideoFrame imageFrame,
        string deviceName)
    ``` 
    And inside function BindModel, we could specify the device, on which we tensorize.
    ```C++
    if ("GPU" == deviceName)
    {
        deviceKind = LearningModelDeviceKind::DirectX;
        inputTensor = TensorizationHelper::LoadInputImageFromGPU(imageFrame.SoftwareBitmap());
    }
    else 
    {
        deviceKind = LearningModelDeviceKind::Default;
        inputTensor = TensorizationHelper::LoadInputImageFromCPU(imageFrame.SoftwareBitmap());
    }
    ```
    The outputs of the model will be saved to disk for check.

2. TensorConvertor.cpp
   > has the implementations of tensorization on both CPU and GPU
   ```C++
   winrt::Windows::AI::MachineLearning::TensorFloat SoftwareBitmapToSoftwareTensor(
        winrt::Windows::Graphics::Imaging::SoftwareBitmap softwareBitmap);

    winrt::Windows::AI::MachineLearning::TensorFloat SoftwareBitmapToDX12Tensor(
        winrt::Windows::Graphics::Imaging::SoftwareBitmap softwareBitmap);
   ```
