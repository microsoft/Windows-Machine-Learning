
# WinMLRunner tool

The WinMLRunner is a command-line based tool that can run .onnx or .pb models where the input and output variables are tensors or images. It will attempt to load, bind, and evaluate a model and output error messages if these steps were unsuccessful. It will also capture performance measurements on the GPU and/or CPU. If using the performance flag, the GPU, CPU and wall-clock times for loading, binding, and evaluating and the CPU and GPU memory usage during evaluation will print to the command line and to a CSV file.

## Prerequisites
- [Visual Studio 2017 Version 15.7.4 or Newer](https://developer.microsoft.com/en-us/windows/downloads)
- [Windows 10 - Build 17738 or higher](https://www.microsoft.com/en-us/software-download/windowsinsiderpreviewiso)
- [Windows SDK - Build 17738 or higher](https://www.microsoft.com/en-us/software-download/windowsinsiderpreviewSDK)

## Build the tool

The easiest way to use these samples without using Git is to download the zip file containing the current version (using the following link or by clicking the "Download ZIP" button on the repo page). You can then unzip the entire archive and use the samples in Visual Studio 2017. Notes: Before you unzip the archive, right-click it, select Properties, and then select Unblock.
Be sure to unzip the entire archive, and not just individual samples. The samples all depend on the SharedContent folder in the archive. In Visual Studio 2017, the platform target defaults to ARM, so be sure to change that to x64 or x86 if you want to test on a non-ARM device. Reminder: If you unzip individual samples, they will not build due to references to other portions of the ZIP file that were not unzipped. 
You must unzip the entire archive if you intend to build the samples.

## Run the tool
 ```
Required command-Line arguments:
-model <path>            : Fully qualified path to a .onnx or .pb model file.
      or
-folder <path>           : Fully qualifed path to a folder with .onnx and/or .pb models, will run all of the models in the folder.

#Optional command-line arguments:
-perf                    : Captures GPU, CPU, and wall-clock time measurements. 
-iterations <int>        : Number of times to evaluate the model when capturing performance measurements.
-CPU                     : Will create a session on the CPU.
-GPU                     : Will create a session on the GPU.
-GPUHighPerformance      : Will create a session with the most powerful GPU device available.
-GPUMinPower             : Will create a session with GPU with the least power.
-CPUBoundInput           : Will bind the input to the CPU.
-GPUBoundInput           : Will bind the input to the GPU.
-BGR                     : Will load the input as a BGR image.
-RGB                     : Will load the input as an RGB image.
-tensor                  : Will load the input as a tensor.
-input <image/CSV path>  : Will bind image/data from CSV to model.
-output <CSV path>       : Path to the CSV where the perf results will be written.
-IgnoreFirstRun          : Will ignore the first run in the perf results when calculating the average
-silent                  : Silent mode (only errors will be printed to the console)
-debug                   : Will start a trace logging session.

 ```

Note that -CPU, -GPU, -GPUHighPerformance, -GPUMinPower -BGR, -RGB, -tensor, -CPUBoundInput, -GPUBoundInput are not mutually exclusive (i.e. you can combine as many as you want to run the model with different configurations).

### Examples:
Run a model on the CPU and GPU separately 5 times and output performance data:
> WinMLRunner.exe -model c:\\data\\concat.onnx -iterations 5 -perf

Runs all the models in the data folder, captures performance data 3 times using only the CPU: 
> WinMLRunner.exe -folder c:\\data -perf -iterations 3 -CPU

Run a model on the CPU and GPU separately, and by binding the input to the CPU and the GPU separately (4 total runs):
> WinMLRunner.exe -model c:\\data\\SqueezeNet.onnx -CPU -GPU -CPUBoundInput -GPUBoundInput

Run a model on the CPU with the input bound to the GPU and loaded as an RGB image:
> WinMLRunner.exe -model c:\\data\\SqueezeNet.onnx -CPU -GPUBoundInput -RGB

## Default output

**Running a good model:**
Run the executable as shown below. Make sure to replace the install location with what matches yours:
 ```
.\WinMLRunner.exe -model SqueezeNet.onnx
WinML Runner
GPU: AMD Radeon Pro WX 3100

Loading model (path = SqueezeNet.onnx)...
=================================================================
Name: squeezenet_old
Author: onnx-caffe2
Version: 9223372036854775807
Domain:
Description:
Path: SqueezeNet.onnx
Support FP16: false

Input Feature Info:
Name: data_0
Feature Kind: Float

Output Feature Info:
Name: softmaxout_1
Feature Kind: Float

=================================================================

Binding (device = CPU, iteration = 1, inputBinding = CPU, inputDataType = Tensor)...[SUCCESS]
Evaluating (device = CPU, iteration = 1, inputBinding = CPU, inputDataType = Tensor)...[SUCCESS]
Outputting results..
Feature Name: softmaxout_1
 resultVector[818] has the maximal value of 1


Binding (device = GPU, iteration = 1, inputBinding = CPU, inputDataType = Tensor)...[SUCCESS]
Evaluating (device = GPU, iteration = 1, inputBinding = CPU, inputDataType = Tensor)...[SUCCESS]
Outputting results..
Feature Name: softmaxout_1
 resultVector[818] has the maximal value of 1
 ```
**Running a bad model:**
Here's an example of running a model with incorrect parameters:
 ```
=================================================================
Name: bad_model
Author: 
Version: 0
Domain: onnxml
Description:
Path: C:\bad_model.onnx
=================================================================

Loading model...[SUCCESS]
Binding Model on GPU...[SUCCESS]
Evaluating Model on GPU...[FAILED]
c:\winml\dll\mloperatorauthorimpl.cpp(1081)\Windows.AI.MachineLearning.dll!00007FFEBBF9E19D: (caller: 00007FFEBC308C37) Exception(4) tid(5190) 80070057 The parameter is incorrect.
    [winrt::Windows::AI::MachineLearning::implementation::AbiOpKernel::Compute::<lambda_0245d72c2a362d137084c22c5297e48a>::operator ()(m_operatorFactory->CreateKernel(kernelInfoWrapper.Get(), ret.GetAddressOf()))]
 ```

## Performance output
The following performance measurements will be output to the command-line and .csv file for each load, bind, and evaluate operation:

* **Wall-clock time**: the elapsed real time (in ms) between the beginning and end of an operation.
* **GPU time**: time (in ms) for an operation to be passed from CPU to GPU and execute on the GPU (note: Load() is not executed on the GPU).
* **CPU time**: time (in ms) for an operation to execute on the CPU. 
 ```
 // The timer class is used to capture high-resolution timing information.
 // TimerHelper.h Line 18
class Timer
{
public:
    void Start()
    {
        LARGE_INTEGER t;
        QueryPerformanceCounter(&t);
        m_startTime = static_cast<double>(t.QuadPart);
    }

    double Stop()
    {
        LARGE_INTEGER stopTime;
        QueryPerformanceCounter(&stopTime);
        double t = static_cast<double>(stopTime.QuadPart) - m_startTime;
        LARGE_INTEGER tps;
        QueryPerformanceFrequency(&tps);
        return t / static_cast<double>(tps.QuadPart) * 1000;
    }

private:
    double m_startTime;
};
 ```
 * **Memory usage (evaluate)**: Average kernel and user-level memory usage (in MB) during evaluate on the CPU or GPU.

 Run the executable as shown below. Make sure to replace the install location with what matches yours:
 ```
WinMLRunner.exe -model C:\Repos\Windows-Machine-Learning\SharedContent\models\SqueezeNet.onnx -perf

If capturing performance measurements, the program will create a CSV titled "WinML Model Run [Today's date]" in the same folder as the .exe file. 
 ```
Working Set Memory (MB) - The amount of DRAM memory that the process on the CPU required during evaluation.
Dedicated Memory (MB) - The amount of memory that was used on the VRAM of the dedicated GPU.
Shared Memory (MB) -  The amount of memory that was used on the DRAM by the GPU.
 ### Sample performance output:
 ```
.\WinMLRunner.exe -model SqueezeNet.onnx -perf
WinML Runner
GPU: AMD Radeon Pro WX 3100

Loading model (path = SqueezeNet.onnx)...
=================================================================
Name: squeezenet_old
Author: onnx-caffe2
Version: 9223372036854775807
Domain:
Description:
Path: SqueezeNet.onnx
Support FP16: false

Input Feature Info:
Name: data_0
Feature Kind: Float

Output Feature Info:
Name: softmaxout_1
Feature Kind: Float

=================================================================

Binding (device = CPU, iteration = 1, inputBinding = CPU, inputDataType = Tensor)...[SUCCESS]
Evaluating (device = CPU, iteration = 1, inputBinding = CPU, inputDataType = Tensor)...[SUCCESS]
Outputting results..
Feature Name: softmaxout_1
 resultVector[818] has the maximal value of 1


Results (device = CPU, numIterations = 1, inputBinding = CPU, inputDataType = Tensor):
  Load: 408.386300 ms
  Bind: 0.9184 ms
  Evaluate: 739.173 ms
  Total Time: 1148.48 ms
  Wall-Clock Load: 408.064 ms
  Wall-Clock Bind: 1.1311 ms
  Wall-Clock Evaluate: 739.337 ms
  Total Wall-Clock Time: 1148.53 ms
  Working Set Memory usage (evaluate): 0 MB
  Dedicated Memory Usage (evaluate): 0 MB
  Shared Memory Usage (evaluate): 0 MB



Binding (device = GPU, iteration = 1, inputBinding = CPU, inputDataType = Tensor)...[SUCCESS]
Evaluating (device = GPU, iteration = 1, inputBinding = CPU, inputDataType = Tensor)...[SUCCESS]
Outputting results..
Feature Name: softmaxout_1
 resultVector[818] has the maximal value of 1


Results (device = GPU, numIterations = 1, inputBinding = CPU, inputDataType = Tensor):
  Load: N/A
  Bind: 3.6711 ms
  Evaluate: 66.5285 ms
  Total Time: 70.1996 ms
  Wall-Clock Load: 0 ms
  Wall-Clock Bind: 3.9697 ms
  Wall-Clock Evaluate: 67.2518 ms
  Total Wall-Clock Time: 71.2215 ms
  Working Set Memory usage (evaluate): 13.668 MB
  Dedicated Memory Usage (evaluate): 13.668 MB
  Shared Memory Usage (evaluate): 1 MB
 ```
 
 ## Capturing Trace Logs
 If you want to capture trace logs using the tool, you can use logman commands in conjunction with the debug flag:
 ```
logman start winml -ets -o winmllog.etl -nb 128 640 -bs 128
logman update trace  winml -p {BCAD6AEE-C08D-4F66-828C-4C43461A033D} 0xffffffffffffffff 0xff -ets         
WinMLRunner.exe -model C:\Repos\Windows-Machine-Learning\SharedContent\models\SqueezeNet.onnx -debug
logman stop winml -ets
 ```
The winmllog.etl file will appear in the same directory as the WinMLRunner.exe. 
 
 ## Reading the Trace Logs
1. Using the traceprt.exe
  * Run the following command from the command-line and open the logdump.csv file:
 ```
tracerpt.exe winmllog.etl -o logdump.csv -of CSV
 ```

2. Windows Performance Analyzer (from Visual Studio)
 * Launch Windows Performance Analyzer and open the winmllog.etl.

## Dynamic DLL Loading

If you want to run WinMLRunner with another version of WinML (e.g. comparing the performance with an older version or testing a newer version), simply place the `windows.ai.machinelearning.dll` and `directml.dll` files in the same folder as WinMLRunner.exe. WinMLRunner will look for for these DLLs first and fall back to `C:/Windows/System32` if it doesn't find them.

## Known issues

- Sequence/Map inputs are not supported yet (the model is just skipped, so it doesn't block other models in a folder);
- We cannot reliably run multiple models with the `-folder` argument with real data. Since we can only specify 1 input, the size of the input would mismatch with most of the models. Right now, using the `-folder` argument only works well with garbage data;
- Generating garbage input as Gray or YUV is not currently supported. Ideally, WinMLRunner's garbage data pipeline should support all inputs types that we can give to winml.

## License
MIT. See [LICENSE file](https://github.com/Microsoft/Windows-Machine-Learning/blob/master/LICENSE).
