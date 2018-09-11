
# WinML Runner Sample

The WinMLRunner program can run .onnx or .pb models where the input and output variables are tensors or images. It will attempt to load, bind, and evaluate a model and output error messages if these steps were unsuccessful. It will also capture performance measurements on the GPU and/or CPU. If using the performance flag, the GPU, CPU and wall-clock times for loading (on CPU only), binding, and evaluating and the CPU and GPU memory usage during evaluate will print to the command line and to a CSV file.

## Prerequisites
- [Visual Studio 2017 Version 15.7.4 or Newer](https://developer.microsoft.com/en-us/windows/downloads)
- [Windows 10 - Build 17728 or higher](https://www.microsoft.com/en-us/software-download/windowsinsiderpreviewiso)
- [Windows SDK - Build 17723 or higher](https://www.microsoft.com/en-us/software-download/windowsinsiderpreviewSDK)

## Build the sample

The easiest way to use these samples without using Git is to download the zip file containing the current version (using the following link or by clicking the "Download ZIP" button on the repo page). You can then unzip the entire archive and use the samples in Visual Studio 2017. Notes: Before you unzip the archive, right-click it, select Properties, and then select Unblock.
Be sure to unzip the entire archive, and not just individual samples. The samples all depend on the SharedContent folder in the archive. In Visual Studio 2017, the platform target defaults to ARM, so be sure to change that to x64 or x86 if you want to test on a non-ARM device. Reminder: If you unzip individual samples, they will not build due to references to other portions of the ZIP file that were not unzipped. 
You must unzip the entire archive if you intend to build the samples.

## Run the sample
 ```
Required command-Line arguments:
-model <path>         : Fully qualified path to a .onnx or .pb model file.
      or
-folder <path>        : Fully qualifed path to a folder with .onnx and/or .pb models, will run all of the models in the folder.

#Optional command-line arguments:
-perf                    : Captures GPU, CPU, and wall-clock time measurements. 
-iterations <int>	     : Number of times to evaluate the model when capturing performance measurements.
-CPU             	     : Will create a session on the CPU.
-GPU            	     : Will create a session on the GPU.
-GPUMaxPerformance     : Will create a session with the most powerful GPU device available.
-GPUMinPower           : Will create a session with GPU with the least power.
-input <image/CSV path>  : Will bind image/data from CSV to model.
-debug                   : Will start a trace logging session. 
 ```
### Examples:
Run a model on the CPU and GPU separately 5 times and output performance data:
> WinMLRunner.exe -model c:\\data\\concat.onnx -iterations 5 -perf

Runs all the models in the data folder, captures performance data 3 times using only the CPU: 
> WinMLRunner .exe -folder c:\\data -perf -iterations 3 -CPU

## Default output

**Running a good model:**
Run the executable as shown below. Make sure to replace the install location with what matches yours:
 ```
 WinMLRunner.exe -model C:\Repos\Windows-Machine-Learning\SharedContent\models\SqueezeNet.onnx 

WinML Runner
GPU: AMD Radeon Pro WX 3100

=================================================================
Name: squeezenet_old
Author: onnx-caffe2
Version: 9223372036854775807
Domain:
Description:
Path: C:\Repos\Windows-Machine-Learning\SharedContent\models\SqueezeNet.onnx
Support FP16: false

Input Feature Info:
Name: data_0
Feature Kind: Float

Output Feature Info:
Name: softmaxout_1
Feature Kind: Float

=================================================================

Loading model...[SUCCESS]
Binding Model on GPU...[SUCCESS]
Evaluating Model on GPU...[SUCCESS]

=================================================================
Name: squeezenet_old
Author: onnx-caffe2
Version: 9223372036854775807
Domain:
Description:
Path: C:\Repos\Windows-Machine-Learning\SharedContent\models\SqueezeNet.onnx
Support FP16: false

Input Feature Info:
Name: data_0
Feature Kind: Float

Output Feature Info:
Name: softmaxout_1
Feature Kind: Float

=================================================================

Loading model...[SUCCESS]
Binding Model on CPU...[SUCCESS]
Evaluating Model on CPU...[SUCCESS]
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
WinML Runner
GPU: AMD Radeon Pro WX 3100

=================================================================
Name: squeezenet_old
Author: onnx-caffe2
Version: 9223372036854775807
Domain:
Description:
Path: C:\Repos\Windows-Machine-Learning\SharedContent\models\SqueezeNet.onnx
Support FP16: false

Input Feature Info:
Name: data_0
Feature Kind: Float

Output Feature Info:
Name: softmaxout_1
Feature Kind: Float

=================================================================

Loading model...[SUCCESS]
Binding Model on GPU...[SUCCESS]
Evaluating Model on GPU...[SUCCESS]

Wall-clock Time Averages (iterations = 1):
  Load: 391.556 ms
  Bind: 10.8784 ms
  Evaluate: 72.4004 ms
  Total time: 474.834 ms


GPU Time Averages (iterations = 1):
  Load: N/A
  Bind: 11.0698 ms
  Evaluate: 72.8877 ms
  Total time: 83.9575 ms
  Dedicated memory usage (evaluate): 13.668 MB
  Shared memory usage (evaluate): 1 MB


=================================================================
Name: squeezenet_old
Author: onnx-caffe2
Version: 9223372036854775807
Domain:
Description:
Path: C:\Repos\Windows-Machine-Learning\SharedContent\models\SqueezeNet.onnx
Support FP16: false

Input Feature Info:
Name: data_0
Feature Kind: Float

Output Feature Info:
Name: softmaxout_1
Feature Kind: Float

=================================================================

Loading model...[SUCCESS]
Binding Model on CPU...[SUCCESS]
Evaluating Model on CPU...[SUCCESS]

Wall-clock Time Averages (iterations = 1):
  Load: 117.31 ms
  Bind: 7.233 ms
  Evaluate: 762.886 ms
  Total time: 887.428 ms


CPU Time Averages (iterations = 1):
  Load: 117.581 ms
  Bind: 7.467 ms
  Evaluate: 690.129 ms
  Total time: 815.176 ms
  Working Set Memory usage (evaluate): 9.77734 MB
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
## License
MIT. See [LICENSE file](https://github.com/Microsoft/Windows-Machine-Learning/blob/master/LICENSE).