# Debugging Your Model

Debugging inference on your onnx model can be useful for, but is not limited to the following use cases:

1. Gain insight into how raw data is flowing through an operator in your model.
2. Compare execution results on different hardware (CPU vs GPU).
3. Visualize intermediate data for computer vision inference.

## The Debug Operator
Gaining access to intermediate data as it flows through your model during inference is achieved using a custom "Debug" operator. This operator is intended to be added into your model and consume a specific intermediate output as its input. It is further configurable with two additional attributes:
1. file_type: [png|text] The file format to export intermediate outputs.

	*png*: interprets and exports intermediate output as an image and requires tensor data in NCHW format. This is useful for computer vision scenarios where you seek to gain additional visual insight as to how features are being extracted through a model.

	*text*: outputs raw tensor data in human readable format.

2. file_path: The file path to export intermediate outputs. Note that the parent directory of this file must exist. 
		It is recommended for png Debug operators consuming tensor data with many channels to output to its own directory since a png will be created for each channel.

## How to use the Debug Operator
The Debug operator is planned be added to a future release of onnx runtime, but until then you will need to compile custom operator code into your app.
1. Compile [debug_cpu.cpp](desktop/cpp/operators/debug_cpu.cpp) and [debug_cpu.h](desktop/cpp/operators/debug_cpu.h) into your project.
2. Compile [customoperatorprovider.h](desktop/cpp/operators/customoperatorprovider.h) into your project but first modify the RegisterSchemas and RegisterKernels subroutines by removing any non DebugOperatorFactory functions and removing the include statements for relu_cpu.h and noisyrelu_cpu.h.
3. When you initialize your LearningModel, pass in a custom provider like so:
    ```
    auto customOperatorProvider = winrt::make<CustomOperatorProvider>();
    auto provider = customOperatorProvider.as<ILearningModelOperatorProvider>();
    auto model = LearningModel::LoadFromFilePath(modelPath, provider);
    ```
4. Modify your input model to include the Debug operator by using [debug_single_output.py](customize_model/scripts/debug_single_output.py) or your own custom method. This script will insert a Debug operator into the provided onnx model. Configuration of the Debug operator is specified via command line arguments:
    ```
    usage: debug_single_output.py [-h] --model_path MODEL_PATH --modified_path
                              MODIFIED_PATH --debug_file_type {png,text}
                              --debug_file_path DEBUG_FILE_PATH
                              --intermediate_output INTERMEDIATE_OUTPUT
    ```
    You can also use [debug_all_outputs.py](customize_model/scripts/debug_all_outputs.py) to interlace Debug operators into every intermediate output of a graph.
Here are netron visualizations of SqueezeNet's first section before and after running these scripts:


    | | | |
    |:-------------------------:|:-------------------------:|:-------------------------:|
    |<img width="1604" src=customize_model/img/squeezenet.png>  Original | <img width="1604" src="customize_model/img/squeezenet_debug_single_output.png">[debug_single_output.py](customize_model/scripts/debug_single_output.py)|<img width="1604" src="customize_model/img/squeezenet_debug_all_outputs.png">[debug_all_outputs.py](customize_model/scripts/debug_all_outputs.py)|
    

All together, you are defining and passing in to your LearningModel a custom operator provider which recognizes the Debug operator. Then you are configuring your input onnx model to contain these Debug operators as required for your specific debugging use case.