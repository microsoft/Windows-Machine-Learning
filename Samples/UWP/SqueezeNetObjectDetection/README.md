
# SqueezeNet Object Detection Sample

A simple UWP application that uses a pretrained machine learning model called SqueezeNet to detect the predominant object in an image selected by the user either from file.

This sample demonstrates the use of generic Windows.AI.MachineLearning.Preview API to load a model, bind an input image and an output tensor, and evaluate a binding. You can use Netron to determine the input and output requirements of your ONNX model which are presumed to be known in this particular sample. https://github.com/lutzroeder/Netron



## Build the sample


1. If you download the samples ZIP, be sure to unzip the entire archive, not just the folder with
   the sample you want to build.

2. Start Microsoft Visual Studio 2017 and select **File** \> **Open** \> **Project/Solution**.

3. Starting in the folder where you unzipped the samples, go to the Samples subfolder, then the
   subfolder for this specific sample. Double-click the Visual Studio project file (.csproj) file.

4. Press Ctrl+Shift+B, or select **Build** \> **Build Solution**.



## Run the sample



The next steps depend on whether you just want to deploy the sample or you want to both deploy and
run it.



### Deploying the sample

- Select Build > Deploy Solution. 



### Deploying and running the sample

- To debug the sample and then run it, press F5 or select Debug >  Start Debugging. To run the sample without debugging, press Ctrl+F5 or selectDebug > Start Without Debugging.