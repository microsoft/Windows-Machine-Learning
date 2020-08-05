# Sample of Using Batch

## Overview

This sample demonstrates how to override named dimensions to concrete values using LearningModelSessionOptions in order to optimize model performance. This feature will first be released in the 1.5 WinML redistributable. 

## Requirements

- [Visual Studio 2017 - 15.4 or higher](https://developer.microsoft.com/en-us/windows/downloads)
- [Windows 10 - Build 18362 or higher](https://www.microsoft.com/en-us/software-download/windowsinsiderpreviewiso)
- [Windows SDK - Build 18362 or higher](https://www.microsoft.com/en-us/software-download/windowsinsiderpreviewSDK)
- Visual Studio Extension for C++/WinRT

  Do the following to add the C++/WinRT extension in Visual Studio.
  1. Go to **Tools > Extensions and Updates**. 
  2. Select **Online** in the left pane and search for "WinRT" using the search box.
  3. Select the **C++/WinRT** extension, click **Download**, and close Visual Studio. The extension should install automatically.
  4. When the extension has finished installing, re-open Visual Studio.


## Build the sample

1. If you download the samples ZIP, be sure to unzip the entire archive, not just the folder with the sample you want to build.
2. Start Microsoft Visual Studio 2017 and select **File > Open > Project/Solution**.
3. Starting in the folder where you unzipped the samples, go to the **Samples** subfolder, then the subfolder for this specific sample (**Sample\BatchSupport**). Double-click the Visual Studio solution file (BatchSupport.sln).
4. Confirm that the project is pointed to the correct SDK that you installed (e.g. 18362). You can do this by right-clicking the project in the **Solution Explorer**, selecting **Properties**, and modifying the **Windows SDK Version**.
5. Confirm that you are set for the right configuration and platform (for example: Debug, x64).
6. Build the solution (**Ctrl+Shift+B**).

## Run the sample

1. Open a Command Prompt (in the Windows 10 search bar, type **cmd** and press **Enter**).
2. Change the current folder to the folder containing the built EXE (`cd <path-to-exe>`).
3. Run the executable as shown below. Make sure to replace the install location with what matches yours:
  ```
  NamedDimensionOverrides.exe
  ```
4. You should get output similar to the following:
    ```
    Loading modelfile 'D:\repos\Windows-Machine-Learning\Samples\NamedDimensionOverrides\x64\Release\candy.onnx' on the CPU
    Binding...
    Running the model...
    model run took 1156 ticks
    ```

## Key Step Walkthrough

### 1. Create Session with named dimension override session option:
```C++
    LearningModelSessionOptions options;
    options.OverrideNamedDimension(L"None", static_cast<uint32_t>(imageNames.size()));

    LearningModelSession session = LearningModelSession(model, device, options);
```

### 2. Bind Inputs
```C++
    // Load input that match the overriden dimension
    std::vector<VideoFrame> inputFrames = {};
    for (hstring imageName : imageNames) {
        auto imagePath = static_cast<hstring>(FileHelper::GetModulePath().c_str()) + imageName;
        auto imageFrame = FileHelper::LoadImageFile(imagePath);
        inputFrames.emplace_back(imageFrame);
    }

    // bind the inputs to the session
    LearningModelBinding binding(session);
    auto inputFeatureDescriptor = model.InputFeatures().First();
    binding.Bind(inputFeatureDescriptor.Current().Name(), winrt::single_threaded_vector(std::move(inputFrames)));
```